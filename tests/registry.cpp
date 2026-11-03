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
using Type=MDS_ConfigType;
using Access=MDS_ConfigAccess;
const MDS_ConfigSetting definitions[]={
 {"schema",Type::Unsigned,Access::ReadOnly,true,nullptr,1,1,1,true},
 {"name",Type::Text,Access::ReadWrite,true,"Time Machine",0,1,32},
 {"runtime",Type::Unsigned,Access::ReadWrite,false,nullptr,5,0,100},
 {"hardware",Type::Text,Access::ReadOnly,false,"Rev A",0,1,10}
};
int main(){
 MDS_Config config;
 size_t count=1;assert(!config.settings(count)&&count==0);
 assert(config.setSettings(definitions));assert(config.settings(count)==definitions&&count==4);
 assert(strcmp(config.get("name").as<const char*>(),"Time Machine")==0);
 assert(config.get("schema").as<int>()==1);
 JsonDocument input;input["v"]="New Name";
 assert(config.set("name",input["v"]));
 assert(!config.set("schema",input["v"]));assert(!config.set(nullptr,input["v"]));assert(!config.set("unknown",input["v"]));
 for(const char* bad:{"","123456789012345678901234567890123"}){input["v"]=bad;assert(!config.set("name",input["v"]));}
 input["v"]=123;assert(!config.set("name",input["v"]));assert(!config.set("runtime",input["v"]));
 input["v"]=-1;assert(!config.set("runtime",input["v"]));
 input["v"]=99;assert(config.set("runtime",input["v"]));
 assert(config.setup());assert(config.save());
 JsonDocument saved;assert(!deserializeJson(saved,LittleFS.files["/config.json"]));assert(saved.size()==2);assert(saved["schema"].as<int>()==1);assert(saved["runtime"].isNull());
 MDS_Config restored;assert(restored.setSettings(definitions));assert(restored.setup());assert(restored.load());assert(strcmp(restored.get("name").as<const char*>(),"New Name")==0);assert(restored.get("runtime").as<int>()==5);
 for(const char* bad:{"{}","[]","{\"schema\":2,\"name\":\"Bad\"}","{\"schema\":\"1\"}","{\"schema\":1,\"name\":null}","{\"schema\":1,\"name\":false}","{\"schema\":1,\"name\":\"\"}","{\"schema\":1,\"name\":\"A\\u0000B\"}"}){
  LittleFS.files["/config.json"]=bad;assert(!restored.load());assert(strcmp(restored.get("name").as<const char*>(),"New Name")==0);assert(strlen(restored.lastError())>0);
 }
 LittleFS.files["/config.json"]="{\"schema\":1,\"name\":\"Legacy\",\"runtime\":87,\"unknown\":true}";
 assert(restored.load());assert(strcmp(restored.get("name").as<const char*>(),"Legacy")==0);assert(restored.get("runtime").as<int>()==5);
 LittleFS.files["/config.json"]="{\"schema\":1}";assert(restored.load());assert(strcmp(restored.get("name").as<const char*>(),"Legacy")==0);
 bool callback=false;
 restored.setSetConfigCallback([&](JsonDocument&){callback=true;});assert(strstr(restored.lastError(),"mix"));
 restored.setGetConfigCallback([&](JsonDocument&){callback=true;});assert(strstr(restored.lastError(),"mix"));
 assert(restored.load());assert(restored.save());assert(!callback);
 MDS_Config legacy;legacy.setSetConfigCallback([](JsonDocument&){});assert(!legacy.setSettings(definitions));
 legacy.setSetConfigCallback(nullptr);legacy.setGetConfigCallback([](JsonDocument&){});assert(!legacy.setSettings(definitions));
 legacy.setGetConfigCallback(nullptr);assert(legacy.setSettings(definitions));
 auto invalid=definitions[0];invalid.defaultUnsigned=2;assert(!restored.setSettings(&invalid,1));
 invalid=definitions[0];invalid.key=nullptr;assert(!restored.setSettings(&invalid,1));
 invalid=definitions[0];invalid.minimum=2;assert(!restored.setSettings(&invalid,1));
 invalid=definitions[0];invalid.persistent=false;assert(!restored.setSettings(&invalid,1));
 MDS_ConfigSetting duplicates[]={definitions[0],definitions[0]};assert(!restored.setSettings(duplicates));
 assert(!restored.setSettings(nullptr,0));assert(restored.get("schema").as<int>()==1);assert(strcmp(restored.get("name").as<const char*>(),"Legacy")==0);
 // Read-only does not imply persisted or required. Persisted read-only values are validated but never imported.
 const MDS_ConfigSetting readOnly[]={ {"version",Type::Unsigned,Access::ReadOnly,true,nullptr,1,0,10} };
 MDS_Config ro;assert(ro.setSettings(readOnly));assert(ro.setup());LittleFS.files["/config.json"]="{\"version\":2}";assert(ro.load());assert(ro.get("version").as<int>()==1);
 puts("PASS: registry/defaults, RO/RW enforcement, persistence independent of access, strict validation, atomic load rejection, legacy JSON compatibility, metadata rejection, and mixed-mode rejection in both directions.");
}
