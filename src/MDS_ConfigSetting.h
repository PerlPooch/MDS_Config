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

#pragma once

#include <stdint.h>
#include <ArduinoJson.h>

enum class MDS_ConfigType {
	Unsigned,
	Text,
	Boolean,
	Signed
};

enum class MDS_ConfigAccess {
	ReadOnly,
	ReadWrite
};

// Application-owned metadata; the registry and its strings must outlive MDS_Config.
struct MDS_ConfigSetting {
	const char* key;
	MDS_ConfigType type;
	MDS_ConfigAccess access;
	bool persistent;
	const char* defaultText;
	uint32_t defaultUnsigned;
	uint32_t minimum; // Text length or unsigned/boolean value, inclusive.
	uint32_t maximum;
	bool required = false;                         // Require this field when loading a stored document.
	bool sensitive = false;                        // Redact this setting from diagnostic dumps.
	bool (*validator)(JsonVariantConst) = nullptr; // Optional application value check.

	// Signed fields use separate defaults and bounds; older aggregate initializers remain valid.
	int64_t defaultSigned = 0;
	int64_t minimumSigned = INT64_MIN;
	int64_t maximumSigned = INT64_MAX;
};
