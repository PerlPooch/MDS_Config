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

using Type = MDS_ConfigType;
using Access = MDS_ConfigAccess;

const MDS_ConfigSetting settings[] = {
    // Key       Type            Access             Save   Text default  Number  Min  Max
    {"model",    Type::Text,      Access::ReadOnly,   false, "CoffeeBot",  0,      1,   24},
    {"name",     Type::Text,      Access::ReadWrite,  true,  "Kitchen",    0,      1,   24},
    {"cups",     Type::Unsigned,  Access::ReadWrite,  true,  nullptr,      2,      1,   12},
    {"enabled",  Type::Boolean,   Access::ReadWrite,  true,  nullptr,      1,      0,    1}
};

MDS_Config config;

void setup() {
    Serial.begin(115200);

    if (!config.setSettings(settings) || !config.setup()) {
        Serial.println(config.lastError());
        return;
    }

    if (config.exists()) {
        if (!config.load()) {
            Serial.println(config.lastError());
            return;
        }
    } else {
        // First run: change two defaults and save them.
        JsonDocument edits;
        edits["name"] = "Office";
        edits["cups"] = 4;  // Values outside 1–12 are rejected.

        // Validate the whole group before applying any changes.
        if (!config.set(edits) || !config.save()) {
            Serial.println(config.lastError());
            return;
        }
    }

    Serial.print("Cups: ");
    Serial.println(config.get("cups").as<unsigned int>());
    config.dump();  // Show current settings, including read-only values.
}

void loop() {
}
