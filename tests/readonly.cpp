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
  {"pwm",Type::Unsigned,Access::ReadWrite,true,nullptr,255,0,255},
  {"gcc",Type::Unsigned,Access::ReadOnly,false,nullptr,4,0,255}
 };
 MDS_Config config;assert(config.setSettings(settings));
 JsonDocument value;value["v"]=8;
 assert(config.setReadOnly("gcc",value["v"]));assert(config.get("gcc").as<int>()==8);
 assert(!config.set("gcc",value["v"]));
 JsonDocument edits;edits["gcc"]=16;assert(!config.set(edits));assert(config.get("gcc").as<int>()==8);
 assert(!config.setReadOnly("pwm",value["v"]));assert(!config.setReadOnly("unknown",value["v"]));
 value["v"]=256;assert(!config.setReadOnly("gcc",value["v"]));assert(config.get("gcc").as<int>()==8);
 value["v"]="8";assert(!config.setReadOnly("gcc",value["v"]));
 assert(config.begin());assert(config.save());JsonDocument saved;assert(!deserializeJson(saved,LittleFS.files["/config.json"]));assert(saved["gcc"].isNull());assert(saved["pwm"].as<int>()==255);
 LittleFS.files["/config.json"]="{\"schema\":1,\"pwm\":64,\"gcc\":200}";
 assert(config.load());assert(config.get("gcc").as<int>()==8&&config.get("pwm").as<int>()==64);
 puts("PASS: implementation RO updates preserve user-write rejection, enforce types/ranges, and remain independent of persistence.");
}
