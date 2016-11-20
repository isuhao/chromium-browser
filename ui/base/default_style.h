// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DEFAULT_STYLE_H_
#define UI_BASE_DEFAULT_STYLE_H_

#include "build/build_config.h"

// This file contains the constants that provide the default style for UI
// controls and dialogs.

namespace ui {

// Default font size delta for messages in dialogs. Note that on Windows, the
// "base" font size is determined by consulting the system for the font used in
// native MessageBox dialogs. On Mac, it is [NSFont systemFontSize]. Linux
// consults the default font description for a GTK Widget context. On ChromeOS,
// ui::ResourceBundle provides a description via IDS_UI_FONT_FAMILY_CROS.
const int kMessageFontSizeDelta = 0;

// Default font size delta for dialog buttons, textfields, and labels.
#if defined(OS_MACOSX)
// Cocoa dialogs prefer [NSFont smallSystemFontSize] for labels (typically 11pt
// vs 13pt).
const int kLabelFontSizeDelta = -2;
#else
const int kLabelFontSizeDelta = 0;
#endif

// Font size delta for dialog titles.
#if defined(OS_MACOSX)
const int kTitleFontSizeDelta = 0;
#else
const int kTitleFontSizeDelta = 3;
#endif

}  // namespace ui

#endif  // UI_BASE_DEFAULT_STYLE_H_
