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
#include <cstring>
#include <cstdio>
Stream Serial;
FakeFS LittleFS;
int main(){
 MDS_Config config;
 assert(!config.load() && strcmp(config.lastError(),"Filesystem not ready")==0);
 assert(!config.save() && strcmp(config.lastError(),"Filesystem not ready")==0);
 assert(config.setup());assert(!LittleFS.formatted);assert(!config.exists());
 assert(!config.load() && strcmp(config.lastError(),"Load callback not set")==0);
 assert(!config.save() && strcmp(config.lastError(),"Save callback not set")==0);
 int loads=0,saves=0;String name="Original";
 config.setSetConfigCallback([&](JsonDocument& doc){++loads;name=doc["name"].as<const char*>();});
 config.setGetConfigCallback([&](JsonDocument& doc){++saves;doc["name"]=name;});
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
 MDS_Config custom("/custom.json");custom.setGetConfigCallback([](JsonDocument& doc){doc["custom"]=true;});assert(custom.setup(true));assert(LittleFS.formatted);assert(custom.save());assert(LittleFS.exists("/custom.json"));
 config.setSetConfigCallback(nullptr);assert(!config.load());
 config.setGetConfigCallback(nullptr);assert(!config.save());
 MDS_Config failed;LittleFS.mountOK=false;assert(!failed.setup());assert(!failed.exists());
 puts("PASS: unchanged callback APIs, default/custom paths, mount behavior, load/save callbacks and errors, verified writes, write/rename failures, print, and clearing callbacks.");
}
