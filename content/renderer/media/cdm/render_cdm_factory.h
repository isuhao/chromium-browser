// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_CDM_RENDER_CDM_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_CDM_RENDER_CDM_FACTORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "media/base/cdm_factory.h"
#include "media/base/media_keys.h"

class GURL;

namespace media {
struct CdmConfig;
}  // namespace media

namespace content {

// CdmFactory implementation in content/renderer. This class is not thread safe
// and should only be used on one thread.
class RenderCdmFactory : public media::CdmFactory {
 public:
  RenderCdmFactory();

  ~RenderCdmFactory() override;

  // CdmFactory implementation.
  void Create(
      const std::string& key_system,
      const GURL& security_origin,
      const media::CdmConfig& cdm_config,
      const media::SessionMessageCB& session_message_cb,
      const media::SessionClosedCB& session_closed_cb,
      const media::LegacySessionErrorCB& legacy_session_error_cb,
      const media::SessionKeysChangeCB& session_keys_change_cb,
      const media::SessionExpirationUpdateCB& session_expiration_update_cb,
      const media::CdmCreatedCB& cdm_created_cb) override;

 private:
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(RenderCdmFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_CDM_RENDER_CDM_FACTORY_H_
