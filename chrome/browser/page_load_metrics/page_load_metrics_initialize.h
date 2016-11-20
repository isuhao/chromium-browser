// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_

#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"

namespace content {
class WebContents;
}

namespace chrome {

void InitializePageLoadMetricsForWebContents(
    content::WebContents* web_contents);

class PageLoadMetricsEmbedder
    : public page_load_metrics::PageLoadMetricsEmbedderInterface {
 public:
  // PageLoadMetricsEmbedderInterface:
  ~PageLoadMetricsEmbedder() override;
  bool IsPrerendering(content::WebContents* web_contents) override;
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override;
};

}  // namespace chrome

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_
