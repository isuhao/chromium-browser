// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/subprocess_metrics_provider.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "components/metrics/metrics_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

SubprocessMetricsProvider::SubprocessMetricsProvider()
    : scoped_observer_(this) {
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

SubprocessMetricsProvider::~SubprocessMetricsProvider() {}

void SubprocessMetricsProvider::RegisterSubprocessAllocator(
    int id,
    std::unique_ptr<base::PersistentHistogramAllocator> allocator) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!allocators_by_id_.Lookup(id));

  // Map is "MapOwnPointer" so transfer ownership to it.
  allocators_by_id_.AddWithID(allocator.release(), id);
}

void SubprocessMetricsProvider::DeregisterSubprocessAllocator(int id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!allocators_by_id_.Lookup(id))
    return;

  // Extract the matching allocator from the list of active ones. It will
  // be automatically released when this method exits.
  std::unique_ptr<base::PersistentHistogramAllocator> allocator(
      allocators_by_id_.Replace(id, nullptr));
  allocators_by_id_.Remove(id);
  DCHECK(allocator);

  // Merge the last deltas from the allocator before it is released.
  MergeHistogramDeltasFromAllocator(id, allocator.get());
}

void SubprocessMetricsProvider::MergeHistogramDeltasFromAllocator(
    int id,
    base::PersistentHistogramAllocator* allocator) {
  DCHECK(allocator);

  int histogram_count = 0;
  base::PersistentHistogramAllocator::Iterator hist_iter(allocator);
  while (true) {
    std::unique_ptr<base::HistogramBase> histogram = hist_iter.GetNext();
    if (!histogram)
      break;
    allocator->MergeHistogramDeltaToStatisticsRecorder(histogram.get());
    ++histogram_count;
  }

  DVLOG(1) << "Reported " << histogram_count << " histograms from subprocess #"
           << id;
}

void SubprocessMetricsProvider::MergeHistogramDeltas() {
  DCHECK(thread_checker_.CalledOnValidThread());

  for (AllocatorByIdMap::iterator iter(&allocators_by_id_); !iter.IsAtEnd();
       iter.Advance()) {
    MergeHistogramDeltasFromAllocator(iter.GetCurrentKey(),
                                      iter.GetCurrentValue());
  }

  UMA_HISTOGRAM_COUNTS_100(
      "UMA.SubprocessMetricsProvider.SubprocessCount",
      allocators_by_id_.size());
}

void SubprocessMetricsProvider::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(content::NOTIFICATION_RENDERER_PROCESS_CREATED, type);

  content::RenderProcessHost* host =
      content::Source<content::RenderProcessHost>(source).ptr();

  // Sometimes, the same host will cause multiple notifications in tests so
  // could possibly do the same in a release build.
  if (!scoped_observer_.IsObserving(host))
    scoped_observer_.Add(host);
}

void SubprocessMetricsProvider::RenderProcessReady(
    content::RenderProcessHost* host) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // If the render-process-host passed a persistent-memory-allocator to the
  // renderer process, extract it and register it here.
  std::unique_ptr<base::SharedPersistentMemoryAllocator> allocator =
      host->TakeMetricsAllocator();
  if (allocator) {
    RegisterSubprocessAllocator(
        host->GetID(),
        WrapUnique(new base::PersistentHistogramAllocator(
            std::move(allocator))));
  }
}

void SubprocessMetricsProvider::RenderProcessExited(
    content::RenderProcessHost* host,
    base::TerminationStatus status,
    int exit_code) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DeregisterSubprocessAllocator(host->GetID());
}

void SubprocessMetricsProvider::RenderProcessHostDestroyed(
    content::RenderProcessHost* host) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // It's possible for a Renderer to terminate without RenderProcessExited
  // (above) being called so it's necessary to de-register also upon the
  // destruction of the host. If both get called, no harm is done.

  DeregisterSubprocessAllocator(host->GetID());
  scoped_observer_.Remove(host);
}
