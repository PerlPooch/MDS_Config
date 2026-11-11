/*
 * ===========================================================================
 * MDS_Config - https://home.marcspencer.com/mds-config
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
Stream Serial;FakeFS LittleFS;
bool allowed(JsonVariantConst value){return value.as<uint32_t>()%2==0;}
int main(){
 using Type=MDS_ConfigType;using Access=MDS_ConfigAccess;
 const MDS_ConfigSetting settings[]={ {"even",Type::Unsigned,Access::ReadWrite,true,nullptr,2,0,10,false,false,allowed} };
 MDS_Config config;assert(config.setSettings(settings));JsonDocument value;value["v"]=3;assert(!config.set("even",value["v"]));assert(config.get("even").as<int>()==2);
 JsonDocument group;group["even"]=5;assert(!config.set(group));assert(config.get("even").as<int>()==2);
 value["v"]=4;assert(config.set("even",value["v"]));assert(config.begin());LittleFS.files["/config.json"]="{\"even\":7}";assert(!config.load());assert(config.get("even").as<int>()==4);
 auto invalid=settings[0];invalid.defaultUnsigned=3;assert(!config.setSettings(&invalid,1));
 puts("PASS: optional application validators check defaults, single/group edits and persisted loads without partial mutation.");
}
