// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_
#define COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_

// Enum to record the user's default search engine choice in UMA.  Add new
// search engines at the bottom and do not delete from this list, so as not
// to disrupt UMA data already recorded.
enum SearchEngineType {
  // Prepopulated engines.
  SEARCH_ENGINE_UNKNOWN = -1,
  SEARCH_ENGINE_OTHER = 0,   // At the top in case of future list changes.
  SEARCH_ENGINE_AOL,
  SEARCH_ENGINE_ASK,
  SEARCH_ENGINE_ATLAS,
  SEARCH_ENGINE_AVG,
  SEARCH_ENGINE_BAIDU,
  SEARCH_ENGINE_BABYLON,
  SEARCH_ENGINE_BING,
  SEARCH_ENGINE_CONDUIT,
  SEARCH_ENGINE_DAUM,
  SEARCH_ENGINE_DELFI,
  SEARCH_ENGINE_DELTA,
  SEARCH_ENGINE_DUCKDUCKGO,
  SEARCH_ENGINE_FUNMOODS,
  SEARCH_ENGINE_GOO,
  SEARCH_ENGINE_GOOGLE,
  SEARCH_ENGINE_IMINENT,
  SEARCH_ENGINE_IMESH,
  SEARCH_ENGINE_IN,
  SEARCH_ENGINE_INCREDIBAR,
  SEARCH_ENGINE_KVASIR,
  SEARCH_ENGINE_LIBERO,
  SEARCH_ENGINE_MAILRU,
  SEARCH_ENGINE_NAJDI,
  SEARCH_ENGINE_NATE,
  SEARCH_ENGINE_NAVER,
  SEARCH_ENGINE_NETI,
  SEARCH_ENGINE_NIGMA,
  SEARCH_ENGINE_OK,
  SEARCH_ENGINE_ONET,
  SEARCH_ENGINE_RAMBLER,
  SEARCH_ENGINE_SAPO,
  SEARCH_ENGINE_SEARCHNU,
  SEARCH_ENGINE_SEARCH_RESULTS,
  SEARCH_ENGINE_SEZNAM,
  SEARCH_ENGINE_SNAPDO,
  SEARCH_ENGINE_SOFTONIC,
  SEARCH_ENGINE_SOGOU,
  SEARCH_ENGINE_SOSO,
  SEARCH_ENGINE_SWEETPACKS,
  SEARCH_ENGINE_TERRA,
  SEARCH_ENGINE_TUT,
  SEARCH_ENGINE_VINDEN,
  SEARCH_ENGINE_VIRGILIO,
  SEARCH_ENGINE_WALLA,
  SEARCH_ENGINE_WP,
  SEARCH_ENGINE_YAHOO,
  SEARCH_ENGINE_YANDEX,
  SEARCH_ENGINE_ZOZNAM,
  SEARCH_ENGINE_MAX          // Bounding value needed for UMA histogram macro.
};

#endif  // COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_TYPE_H_
