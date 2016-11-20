// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/debug/micro_benchmark_controller_impl.h"

#include <string>

#include "base/callback.h"
#include "base/values.h"
#include "cc/trees/layer_tree_host_impl.h"

namespace cc {

MicroBenchmarkControllerImpl::MicroBenchmarkControllerImpl(
    LayerTreeHostImpl* host)
    : host_(host) {
  DCHECK(host_);
}

MicroBenchmarkControllerImpl::~MicroBenchmarkControllerImpl() {}

void MicroBenchmarkControllerImpl::ScheduleRun(
    std::unique_ptr<MicroBenchmarkImpl> benchmark) {
  benchmarks_.push_back(std::move(benchmark));
}

void MicroBenchmarkControllerImpl::DidCompleteCommit() {
  for (const auto& benchmark : benchmarks_) {
    DCHECK(!benchmark->IsDone());
    benchmark->DidCompleteCommit(host_);
  }

  CleanUpFinishedBenchmarks();
}

void MicroBenchmarkControllerImpl::CleanUpFinishedBenchmarks() {
  benchmarks_.erase(
      std::remove_if(benchmarks_.begin(), benchmarks_.end(),
                     [](const std::unique_ptr<MicroBenchmarkImpl>& benchmark) {
                       return benchmark->IsDone();
                     }),
      benchmarks_.end());
}

}  // namespace cc
