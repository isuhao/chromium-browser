// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_layer_tree_host.h"
#include "cc/test/fake_layer_tree_host_client.h"
#include "cc/test/test_task_graph_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

namespace {

TEST(LayerTreeHostRecordGpuHistogramTest, SingleThreaded) {
  FakeLayerTreeHostClient host_client(FakeLayerTreeHostClient::DIRECT_3D);
  TestTaskGraphRunner task_graph_runner;
  LayerTreeSettings settings;
  std::unique_ptr<FakeLayerTreeHost> host =
      FakeLayerTreeHost::Create(&host_client, &task_graph_runner, settings,
                                CompositorMode::SINGLE_THREADED);
  host->RecordGpuRasterizationHistogram();
  EXPECT_FALSE(host->gpu_rasterization_histogram_recorded());
}

TEST(LayerTreeHostRecordGpuHistogramTest, Threaded) {
  FakeLayerTreeHostClient host_client(FakeLayerTreeHostClient::DIRECT_3D);
  TestTaskGraphRunner task_graph_runner;
  LayerTreeSettings settings;
  std::unique_ptr<FakeLayerTreeHost> host = FakeLayerTreeHost::Create(
      &host_client, &task_graph_runner, settings, CompositorMode::THREADED);
  host->RecordGpuRasterizationHistogram();
  EXPECT_TRUE(host->gpu_rasterization_histogram_recorded());
}

}  // namespace

}  // namespace cc
