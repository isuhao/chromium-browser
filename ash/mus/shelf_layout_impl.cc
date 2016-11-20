// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/mus/shelf_layout_impl.h"

#include "ash/mus/root_window_controller.h"
#include "ash/mus/shelf_layout_manager.h"
#include "ash/mus/status_layout_manager.h"

namespace ash {
namespace mus {

ShelfLayoutImpl::ShelfLayoutImpl() {}

ShelfLayoutImpl::~ShelfLayoutImpl() {}

void ShelfLayoutImpl::Initialize(RootWindowController* root_controller) {
  root_controller_ = root_controller;
}

void ShelfLayoutImpl::SetAlignment(mash::shelf::mojom::Alignment alignment) {
  root_controller_->GetShelfLayoutManager()->SetAlignment(alignment);
  root_controller_->GetStatusLayoutManager()->SetAlignment(alignment);
}

void ShelfLayoutImpl::SetAutoHideBehavior(
    mash::shelf::mojom::AutoHideBehavior auto_hide) {
  root_controller_->GetShelfLayoutManager()->SetAutoHideBehavior(auto_hide);
  root_controller_->GetStatusLayoutManager()->SetAutoHideBehavior(auto_hide);
}

}  // namespace mus
}  // namespace ash
