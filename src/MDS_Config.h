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
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include "MDS_ConfigSetting.h"

class MDS_Config {
public:
	using Callback = std::function<void(JsonDocument&)>;
	explicit MDS_Config(const char* path = "/config.json");
	bool setup(bool formatOnFail = false);
	bool load();
	bool save();
	bool exists() const;
	void print(Stream& output = Serial);

	// Print current registry values/access flags, redacting sensitive settings.
	void dump(Stream& output = Serial) const;
	const char* lastError() const { return error_.c_str(); }
	void setSetConfigCallback(Callback callback);
	void setGetConfigCallback(Callback callback);

	// Opt into managed settings; borrow definitions and initialize validated defaults.
	bool setSettings(const MDS_ConfigSetting* settings, size_t count);

	// Register a static array, inferring its number of entries.
	template<size_t Count>
	bool setSettings(const MDS_ConfigSetting (&settings)[Count]) {
		return setSettings(settings, Count);
	}

	// Enumerate the registered definitions; count is zero in callback mode.
	const MDS_ConfigSetting* settings(size_t& count) const;

	// Read a managed setting; unknown keys return a null variant.
	JsonVariantConst get(const char* key) const;

	// Apply a validated user edit in memory; read-only settings cannot be edited.
	bool set(const char* key, JsonVariantConst value);

	// Validate and apply a group of user edits together; reject the entire group on error.
	bool set(const JsonDocument& updates);

	// Implementation-only update of RO status; user-facing setters still reject RO fields.
	bool setReadOnly(const char* key, JsonVariantConst value);
private:
	bool fail(const char* message);
	const MDS_ConfigSetting* find(const char* key) const;
	static bool valid(const MDS_ConfigSetting& setting, JsonVariantConst value);
	bool loadSettings(const JsonDocument& document);
	bool saveSettings(JsonDocument& document);

	const MDS_ConfigSetting* settings_ = nullptr;
	size_t settingCount_ = 0;
	JsonDocument values_;
	String path_;
	String error_;
	bool ready_ = false;
	Callback setData_;
	Callback getData_;
};
