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
Stream Serial;
FakeFS LittleFS;
int main(){
 using Type=MDS_ConfigType;using Access=MDS_ConfigAccess;
 const MDS_ConfigSetting settings[]={
  {"schema",Type::Unsigned,Access::ReadOnly,true,nullptr,1,1,1,true},
  {"name",Type::Text,Access::ReadWrite,true,"Original",0,1,32},
  {"rate",Type::Unsigned,Access::ReadWrite,true,nullptr,5,0,100}
 };
 MDS_Config config;JsonDocument updates;updates["name"]="Updated";
 assert(!config.set(updates));assert(config.setSettings(settings));
 updates["rate"]=101;assert(!config.set(updates));assert(String(config.get("name").as<const char*>())=="Original");
 updates["rate"]=10;assert(config.set(updates));assert(config.get("rate").as<int>()==10);assert(String(config.get("name").as<const char*>())=="Updated");
 updates["name"]="Rejected";updates["schema"]=1;assert(!config.set(updates));assert(String(config.get("name").as<const char*>())=="Updated");
 updates.remove("schema");updates["unknown"]=true;assert(!config.set(updates));assert(config.get("rate").as<int>()==10);
 updates.remove("unknown");updates["rate"]=nullptr;assert(!config.set(updates));
 updates.to<JsonArray>();assert(!config.set(updates));
 puts("PASS: grouped edits validate all keys/access/types/ranges before commit and reject entire invalid groups.");
}
