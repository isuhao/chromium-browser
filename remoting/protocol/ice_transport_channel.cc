// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/ice_transport_channel.h"

#include <algorithm>
#include <utility>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "jingle/glue/utils.h"
#include "net/base/net_errors.h"
#include "remoting/protocol/channel_socket_adapter.h"
#include "remoting/protocol/port_allocator_factory.h"
#include "remoting/protocol/transport_context.h"
#include "third_party/webrtc/base/network.h"
#include "third_party/webrtc/p2p/base/p2pconstants.h"
#include "third_party/webrtc/p2p/base/p2ptransportchannel.h"
#include "third_party/webrtc/p2p/base/port.h"
#include "third_party/webrtc/p2p/client/httpportallocator.h"

namespace remoting {
namespace protocol {

namespace {

const int kIceUfragLength = 16;

// Utility function to map a cricket::Candidate string type to a
// TransportRoute::RouteType enum value.
TransportRoute::RouteType CandidateTypeToTransportRouteType(
    const std::string& candidate_type) {
  if (candidate_type == "local") {
    return TransportRoute::DIRECT;
  } else if (candidate_type == "stun" || candidate_type == "prflx") {
    return TransportRoute::STUN;
  } else if (candidate_type == "relay") {
    return TransportRoute::RELAY;
  } else {
    LOG(FATAL) << "Unknown candidate type: " << candidate_type;
    return TransportRoute::DIRECT;
  }
}

}  // namespace

IceTransportChannel::IceTransportChannel(
    scoped_refptr<TransportContext> transport_context)
    : transport_context_(transport_context),
      ice_username_fragment_(
          rtc::CreateRandomString(kIceUfragLength)),
      connect_attempts_left_(
          transport_context->network_settings().ice_reconnect_attempts),
      weak_factory_(this) {
  DCHECK(!ice_username_fragment_.empty());
}

IceTransportChannel::~IceTransportChannel() {
  DCHECK(delegate_);

  delegate_->OnChannelDeleted(this);

  auto task_runner = base::ThreadTaskRunnerHandle::Get();
  if (channel_)
    task_runner->DeleteSoon(FROM_HERE, channel_.release());
  if (port_allocator_)
    task_runner->DeleteSoon(FROM_HERE, port_allocator_.release());
}

void IceTransportChannel::Connect(const std::string& name,
                                  Delegate* delegate,
                                  const ConnectedCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!name.empty());
  DCHECK(delegate);
  DCHECK(!callback.is_null());

  DCHECK(name_.empty());
  name_ = name;
  delegate_ = delegate;
  callback_ = callback;

  port_allocator_ =
      transport_context_->port_allocator_factory()->CreatePortAllocator(
          transport_context_);

  // Create P2PTransportChannel, attach signal handlers and connect it.
  // TODO(sergeyu): Specify correct component ID for the channel.
  channel_.reset(new cricket::P2PTransportChannel(
      std::string(), 0, port_allocator_.get()));
  std::string ice_password = rtc::CreateRandomString(cricket::ICE_PWD_LENGTH);
  channel_->SetIceProtocolType(cricket::ICEPROTO_RFC5245);
  channel_->SetIceRole((transport_context_->role() == TransportRole::CLIENT)
                           ? cricket::ICEROLE_CONTROLLING
                           : cricket::ICEROLE_CONTROLLED);
  delegate_->OnChannelIceCredentials(this, ice_username_fragment_,
                                     ice_password);
  channel_->SetIceCredentials(ice_username_fragment_, ice_password);
  channel_->SignalCandidateGathered.connect(
      this, &IceTransportChannel::OnCandidateGathered);
  channel_->SignalRouteChange.connect(
      this, &IceTransportChannel::OnRouteChange);
  channel_->SignalWritableState.connect(
      this, &IceTransportChannel::OnWritableState);
  channel_->set_incoming_only(!(transport_context_->network_settings().flags &
                                NetworkSettings::NAT_TRAVERSAL_OUTGOING));

  channel_->Connect();
  channel_->MaybeStartGathering();

  // Pass pending ICE credentials and candidates to the channel.
  if (!remote_ice_username_fragment_.empty()) {
    channel_->SetRemoteIceCredentials(remote_ice_username_fragment_,
                                      remote_ice_password_);
  }

  while (!pending_candidates_.empty()) {
    channel_->AddRemoteCandidate(pending_candidates_.front());
    pending_candidates_.pop_front();
  }

  --connect_attempts_left_;

  // Start reconnection timer.
  reconnect_timer_.Start(FROM_HERE,
                         transport_context_->network_settings().ice_timeout,
                         this, &IceTransportChannel::TryReconnect);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&IceTransportChannel::NotifyConnected,
                            weak_factory_.GetWeakPtr()));
}

void IceTransportChannel::NotifyConnected() {
  // Create P2PDatagramSocket adapter for the P2PTransportChannel.
  std::unique_ptr<TransportChannelSocketAdapter> socket(
      new TransportChannelSocketAdapter(channel_.get()));
  socket->SetOnDestroyedCallback(base::Bind(
      &IceTransportChannel::OnChannelDestroyed, base::Unretained(this)));
  base::ResetAndReturn(&callback_).Run(std::move(socket));
}

void IceTransportChannel::SetRemoteCredentials(const std::string& ufrag,
                                               const std::string& password) {
  DCHECK(thread_checker_.CalledOnValidThread());

  remote_ice_username_fragment_ = ufrag;
  remote_ice_password_ = password;

  if (channel_)
    channel_->SetRemoteIceCredentials(ufrag, password);
}

void IceTransportChannel::AddRemoteCandidate(
    const cricket::Candidate& candidate) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // To enforce the no-relay setting, it's not enough to not produce relay
  // candidates. It's also necessary to discard remote relay candidates.
  bool relay_allowed = (transport_context_->network_settings().flags &
                        NetworkSettings::NAT_TRAVERSAL_RELAY) != 0;
  if (!relay_allowed && candidate.type() == cricket::RELAY_PORT_TYPE)
    return;

  if (channel_) {
    channel_->AddRemoteCandidate(candidate);
  } else {
    pending_candidates_.push_back(candidate);
  }
}

const std::string& IceTransportChannel::name() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return name_;
}

bool IceTransportChannel::is_connected() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return callback_.is_null();
}

void IceTransportChannel::OnCandidateGathered(
    cricket::TransportChannelImpl* channel,
    const cricket::Candidate& candidate) {
  DCHECK(thread_checker_.CalledOnValidThread());
  delegate_->OnChannelCandidate(this, candidate);
}

void IceTransportChannel::OnRouteChange(
    cricket::TransportChannel* channel,
    const cricket::Candidate& candidate) {
  // Ignore notifications if the channel is not writable.
  if (channel_->writable())
    NotifyRouteChanged();
}

void IceTransportChannel::OnWritableState(cricket::TransportChannel* channel) {
  DCHECK_EQ(channel, static_cast<cricket::TransportChannel*>(channel_.get()));

  if (channel->writable()) {
    connect_attempts_left_ =
        transport_context_->network_settings().ice_reconnect_attempts;
    reconnect_timer_.Stop();

    // Route change notifications are ignored when the |channel_| is not
    // writable. Notify the event handler about the current route once the
    // channel is writable.
    NotifyRouteChanged();
  } else {
    reconnect_timer_.Reset();
    TryReconnect();
  }
}

void IceTransportChannel::OnChannelDestroyed() {
  // The connection socket is being deleted, so delete the transport too.
  delete this;
}

void IceTransportChannel::NotifyRouteChanged() {
  TransportRoute route;

  DCHECK(channel_->best_connection());
  const cricket::Connection* connection = channel_->best_connection();

  // A connection has both a local and a remote candidate. For our purposes, the
  // route type is determined by the most indirect candidate type. For example:
  // it's possible for the local candidate be a "relay" type, while the remote
  // candidate is "local". In this case, we still want to report a RELAY route
  // type.
  static_assert(TransportRoute::DIRECT < TransportRoute::STUN &&
                TransportRoute::STUN < TransportRoute::RELAY,
                "Route type enum values are ordered by 'indirectness'");
  route.type = std::max(
      CandidateTypeToTransportRouteType(connection->local_candidate().type()),
      CandidateTypeToTransportRouteType(connection->remote_candidate().type()));

  if (!jingle_glue::SocketAddressToIPEndPoint(
          connection->remote_candidate().address(), &route.remote_address)) {
    LOG(FATAL) << "Failed to convert peer IP address.";
  }

  const cricket::Candidate& local_candidate =
      channel_->best_connection()->local_candidate();
  if (!jingle_glue::SocketAddressToIPEndPoint(
          local_candidate.address(), &route.local_address)) {
    LOG(FATAL) << "Failed to convert local IP address.";
  }

  delegate_->OnChannelRouteChange(this, route);
}

void IceTransportChannel::TryReconnect() {
  DCHECK(!channel_->writable());

  if (connect_attempts_left_ <= 0) {
    reconnect_timer_.Stop();

    // Notify the caller that ICE connection has failed - normally that will
    // terminate Jingle connection (i.e. the transport will be destroyed).
    delegate_->OnChannelFailed(this);
    return;
  }
  --connect_attempts_left_;

  // Restart ICE by resetting ICE password.
  std::string ice_password = rtc::CreateRandomString(cricket::ICE_PWD_LENGTH);
  delegate_->OnChannelIceCredentials(this, ice_username_fragment_,
                                     ice_password);
  channel_->SetIceCredentials(ice_username_fragment_, ice_password);
}

}  // namespace protocol
}  // namespace remoting
