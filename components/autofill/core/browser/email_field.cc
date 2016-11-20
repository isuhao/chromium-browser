// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/email_field.h"

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_regex_constants.h"
#include "components/autofill/core/browser/autofill_scanner.h"

namespace autofill {

// static
std::unique_ptr<FormField> EmailField::Parse(AutofillScanner* scanner) {
  AutofillField* field;
  if (ParseFieldSpecifics(scanner, base::UTF8ToUTF16(kEmailRe),
                          MATCH_DEFAULT | MATCH_EMAIL, &field)) {
    return base::WrapUnique(new EmailField(field));
  }

  return NULL;
}

EmailField::EmailField(const AutofillField* field) : field_(field) {
}

void EmailField::AddClassifications(
    FieldCandidatesMap* field_candidates) const {
  AddClassification(field_, EMAIL_ADDRESS, kBaseEmailParserScore,
                    field_candidates);
}

}  // namespace autofill
