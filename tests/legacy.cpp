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
#include <cstring>
#include <cstdio>
Stream Serial;
FakeFS LittleFS;

// Exercise load/save callback registration, persistence, errors, and callback clearing.
int main(){
 MDS_Config config;
 assert(!config.load() && strcmp(config.lastError(),"Filesystem not ready")==0);
 assert(!config.save() && strcmp(config.lastError(),"Filesystem not ready")==0);
 assert(config.begin());assert(!LittleFS.formatted);assert(!config.exists());
 assert(!config.load() && strcmp(config.lastError(),"Load callback not set")==0);
 assert(!config.save() && strcmp(config.lastError(),"Save callback not set")==0);
 int loads=0,saves=0;String name="Original";

 config.setLoadCallback([&](JsonDocument& doc) {
  // Copy the loaded value into application-owned state.
  ++loads;
  name = doc["name"].as<const char*>();
 });

 config.setSaveCallback([&](JsonDocument& doc) {
  // Populate the document that the library will write to the file.
  ++saves;
  doc["name"] = name;
 });

 assert(!config.load() && strcmp(config.lastError(),"Configuration file not found")==0);
 assert(config.save());assert(config.exists());assert(saves==1);
 name="Changed";assert(config.load());assert(loads==1 && name=="Original");
 LittleFS.files["/config.json"]="[]";assert(!config.load() && loads==1);
 LittleFS.files["/config.json"]="{";assert(!config.load() && loads==1);
 LittleFS.files["/config.json"]="{\"name\":\"Legacy\"}";assert(config.load());assert(name=="Legacy");
 config.print();
 auto previous=LittleFS.files["/config.json"];
 LittleFS.shortWrite=true;assert(!config.save());assert(LittleFS.files["/config.json"]==previous);assert(!LittleFS.exists("/config.json.tmp"));LittleFS.shortWrite=false;
 LittleFS.renameOK=false;assert(!config.save());assert(LittleFS.files["/config.json"]==previous);LittleFS.renameOK=true;
 LittleFS.writeOK=false;assert(!config.save());LittleFS.writeOK=true;
 LittleFS.readOK=false;assert(!config.load());LittleFS.readOK=true;
 MDS_Config custom("/custom.json");custom.setSaveCallback([](JsonDocument& doc){doc["custom"]=true;});assert(custom.begin(MDS_Config::FormatPolicy::OnMountFailure));assert(LittleFS.formatted);assert(custom.save());assert(LittleFS.exists("/custom.json"));
 config.setLoadCallback(nullptr);assert(!config.load());
 config.setSaveCallback(nullptr);assert(!config.save());
 MDS_Config failed;LittleFS.mountOK=false;assert(!failed.begin());assert(!failed.exists());
 puts("PASS: load/save callback registration, default/custom paths, mount behavior, load/save callbacks and errors, verified writes, write/rename failures, print, and clearing callbacks.");
}
