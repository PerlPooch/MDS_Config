/*
 * ===========================================================================
 * MDS_Config 0.7.0 - https://home.marcspencer.com/mds-config
 *
 * Copyright (c) 2026 Marc D. Spencer.
 * All Rights Reserved under the applicable license.
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 * ===========================================================================
 */

#include <MDS_Config.h>
#include <LittleFS.h>
#include <cassert>
#include <cstdio>

Stream Serial;
FakeFS LittleFS;

// Verify signed values independently of access and persistence, preserving older descriptors.
int main() {
 using Type = MDS_ConfigType;
 using Access = MDS_ConfigAccess;
 const MDS_ConfigSetting definitions[] = {
  {"stored", Type::Signed, Access::ReadWrite, true, nullptr, 0, 0, 0},
  {"volatile", Type::Signed, Access::ReadWrite, false, nullptr, 0, 0, 0},
  {"unsigned", Type::Unsigned, Access::ReadWrite, true, nullptr, 1, 0, 10},
  {"status", Type::Signed, Access::ReadOnly, false, nullptr, 0, 0, 0}
 };
 MDS_Config config;
 assert(config.setSettings(definitions) && config.begin());
 assert(config.get("stored").is<int64_t>() && config.get("stored").as<int64_t>() == 0);
 JsonDocument edit;
 for (int64_t number : {INT64_MIN, int64_t(-432000), int64_t(0), INT64_MAX}) {
  edit["value"] = number;
  assert(config.set("stored", edit["value"]));
  assert(config.get("stored").as<int64_t>() == number);
 }
 for (const char* invalid : {"true", "1.5", "\"-1\"", "18446744073709551615", "null"}) {
  JsonDocument bad;
  assert(!deserializeJson(bad, invalid));
  assert(!config.set("stored", bad.as<JsonVariantConst>()));
  assert(config.get("stored").as<int64_t>() == INT64_MAX);
 }
 edit.clear(); edit["stored"] = -42; edit["unsigned"] = -1;
 assert(!config.set(edit) && config.get("stored").as<int64_t>() == INT64_MAX);
 edit["unsigned"] = 2; edit["volatile"] = -300;
 assert(config.set(edit));
 edit["value"] = -99;
 assert(!config.set("status", edit["value"]) && config.setReadOnly("status", edit["value"]));
 assert(config.save());
 JsonDocument saved;
 assert(!deserializeJson(saved, LittleFS.files["/config.json"]));
 assert(saved["stored"].as<int64_t>() == -42 && saved["volatile"].isUnbound() && saved["status"].isUnbound());
 MDS_Config restored;
 assert(restored.setSettings(definitions) && restored.begin() && restored.load());
 assert(restored.get("stored").as<int64_t>() == -42 && restored.get("volatile").as<int64_t>() == 0);
 auto bounded = definitions[0];
 bounded.defaultSigned = -5; bounded.minimumSigned = -10; bounded.maximumSigned = 10;
 assert(restored.setSettings(&bounded, 1) && restored.get("stored").as<int64_t>() == -5);
 edit["value"] = -11;
 assert(!restored.set("stored", edit["value"]));
 bounded.minimumSigned = 11;
 assert(!restored.setSettings(&bounded, 1));
 bounded.minimumSigned = -4;
 assert(!restored.setSettings(&bounded, 1)); // Default is now outside the declared range.
 puts("PASS: signed 64-bit defaults/bounds, strict integer validation, atomic groups, RO updates, persistence flags, and old descriptor compatibility.");
}
