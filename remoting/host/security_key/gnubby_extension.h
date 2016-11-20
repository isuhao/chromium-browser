// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SECURITY_KEY_GNUBBY_EXTENSION_H_
#define REMOTING_HOST_SECURITY_KEY_GNUBBY_EXTENSION_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "remoting/host/host_extension.h"

namespace remoting {

class ClientSessionDetails;
class HostExtensionSession;

// GnubbyExtension extends HostExtension to enable Security Key support.
class GnubbyExtension : public HostExtension {
 public:
  GnubbyExtension();
  ~GnubbyExtension() override;

  // HostExtension interface.
  std::string capability() const override;
  std::unique_ptr<HostExtensionSession> CreateExtensionSession(
      ClientSessionDetails* client_session_details,
      protocol::ClientStub* client_stub) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GnubbyExtension);
};

}  // namespace remoting

#endif  // REMOTING_HOST_SECURITY_KEY_GNUBBY_EXTENSION_H_
