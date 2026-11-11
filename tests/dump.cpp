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
Stream Serial;
FakeFS LittleFS;
class Capture : public Stream {
public:
 String text;
 size_t write(uint8_t value) override {text+=char(value);return 1;}
};
int main(){
 using Type=MDS_ConfigType;using Access=MDS_ConfigAccess;
 const MDS_ConfigSetting settings[]={
  {"schema",Type::Unsigned,Access::ReadOnly,true,nullptr,1,1,1,true},
  {"name",Type::Text,Access::ReadWrite,true,"Default",0,1,32},
  {"password",Type::Text,Access::ReadWrite,true,"secret-default",0,1,64,false,true},
  {"apiKey",Type::Text,Access::ReadOnly,false,"secret-key",0,1,64,false,true}
 };
 MDS_Config config;Capture empty;config.dump(empty);assert(empty.text.find("no settings registry")!=String::npos);
 assert(config.setSettings(settings));JsonDocument input;input["value"]="Current Name";assert(config.set("name",input["value"]));
 input["value"]="secret-updated";assert(config.set("password",input["value"]));
 Capture output;config.dump(output);
 assert(output.text.find("schema [RO] = 1")!=String::npos);
 assert(output.text.find("name [RW] = \"Current Name\"")!=String::npos);
 assert(output.text.find("password [RW] = [redacted]")!=String::npos);
 assert(output.text.find("apiKey [RO] = [redacted]")!=String::npos);
 assert(output.text.find("secret")==String::npos);
 assert(config.begin());assert(config.save());assert(LittleFS.files["/config.json"].find("secret-updated")!=String::npos);
 Capture stored;config.print(stored);assert(stored.text.find("secret-updated")!=String::npos);
 puts("PASS: current-value/access dump, sensitive field redaction, legacy raw-file print preserved, and secrets still persist normally.");
}
