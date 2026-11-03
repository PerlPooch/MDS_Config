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

#include "MDS_Config.h"
#include <LittleFS.h>
#include <string.h>
#include <utility>

MDS_Config::MDS_Config(const char* path) : path_(path) {}
bool MDS_Config::fail(const char* message) { error_ = message; return false; }
bool MDS_Config::setup(bool formatOnFail) {
	ready_ = LittleFS.begin(formatOnFail);
	if (!ready_) return fail("LittleFS mount failed");
	error_ = "";
	return true;
}
bool MDS_Config::exists() const { return ready_ && LittleFS.exists(path_); }
bool MDS_Config::load() {
	if (!ready_) return fail("Filesystem not ready");
	if (!settings_ && !setData_) return fail("Load callback not set");
	File file = LittleFS.open(path_, "r");
	if (!file) return fail("Configuration file not found");
	JsonDocument doc;
	const auto result = deserializeJson(doc, file);
	file.close();
	if (result) return fail(result.c_str());
	if (!doc.is<JsonObject>()) return fail("Configuration must be an object");
	if (settings_) {
		if (!loadSettings(doc)) return false;
	} else {
		setData_(doc);
	}
	error_ = "";
	return true;
}
bool MDS_Config::save() {
	if (!ready_) return fail("Filesystem not ready");
	if (!settings_ && !getData_) return fail("Save callback not set");
	JsonDocument doc;
	if (settings_) {
		if (!saveSettings(doc)) return false;
	} else {
		getData_(doc);
	}
	if (doc.overflowed() || !doc.is<JsonObject>()) return fail("Invalid configuration document");
	const String temporary = path_ + ".tmp";
	File file = LittleFS.open(temporary, "w");
	if (!file) return fail("Cannot open temporary configuration");
	const size_t expected = measureJson(doc);
	const size_t written = serializeJson(doc, file);
	file.flush();
	file.close();
	// Read back before replacing the previous configuration.
	File check = LittleFS.open(temporary, "r");
	JsonDocument verified;
	bool valid = written == expected && check && check.size() == expected;
	if (valid) valid = !deserializeJson(verified, check);
	check.close();
	if (!valid) {
		LittleFS.remove(temporary);
		return fail("Configuration write verification failed");
	}
	if (!LittleFS.rename(temporary, path_)) return fail("Configuration replacement failed");
	error_ = "";
	return true;
}
void MDS_Config::print(Stream& output) {
	if (!ready_) return;
	File file = LittleFS.open(path_, "r");
	if (!file) return;
	while (file.available()) output.write(file.read());
	file.close();
	output.println();
}

void MDS_Config::setSetConfigCallback(Callback callback) {
	if (settings_) {
		fail("Cannot mix settings registry and callbacks");
		return;
	}

	setData_ = callback;
}

void MDS_Config::setGetConfigCallback(Callback callback) {
	if (settings_) {
		fail("Cannot mix settings registry and callbacks");
		return;
	}

	getData_ = callback;
}

// Enforce the declared type and bounds before invoking any application validator.
bool MDS_Config::valid(const MDS_ConfigSetting& setting, JsonVariantConst value) {
	if (setting.type == MDS_ConfigType::Signed) {
		if (!value.is<int64_t>()) return false;

		const int64_t number = value.as<int64_t>();
		return number >= setting.minimumSigned && number <= setting.maximumSigned
			&& (!setting.validator || setting.validator(value));
	}

	uint32_t size;

	if (setting.type == MDS_ConfigType::Text) {
		if (!value.is<const char*>()) {
			return false;
		}

		const JsonString text = value.as<JsonString>();
		size = text.size();

		if (strlen(text.c_str()) != size) {
			return false;
		}
	} else if (setting.type == MDS_ConfigType::Boolean) {
		if (!value.is<bool>()) {
			return false;
		}
		size = value.as<bool>() ? 1 : 0;
	} else if (setting.type == MDS_ConfigType::Unsigned) {
		if (!value.is<uint32_t>()) {
			return false;
		}

		size = value.as<uint32_t>();
	} else {
		return false;
	}

	return size >= setting.minimum && size <= setting.maximum
		&& (!setting.validator || setting.validator(value));
}

// Validate definitions and prepare defaults without disturbing existing registered values on failure.
bool MDS_Config::setSettings(const MDS_ConfigSetting* settings, size_t count) {
	if (setData_ || getData_) {
		return fail("Cannot mix settings registry and callbacks");
	}

	if (!settings || !count) {
		return fail("Settings registry must not be empty");
	}

	JsonDocument defaults;

	for (size_t index = 0; index < count; ++index) {
		const auto& setting = settings[index];

		if (!setting.key || !*setting.key
			|| (setting.type == MDS_ConfigType::Signed
				? setting.minimumSigned > setting.maximumSigned : setting.minimum > setting.maximum)
			|| (setting.required && !setting.persistent)
			|| (setting.access != MDS_ConfigAccess::ReadOnly
				&& setting.access != MDS_ConfigAccess::ReadWrite)) {
			return fail("Invalid setting definition");
		}

		for (size_t previous = 0; previous < index; ++previous) {
			if (strcmp(settings[previous].key, setting.key) == 0) {
				return fail("Duplicate setting key");
			}
		}

		if (setting.type == MDS_ConfigType::Text) {
			defaults[setting.key] = setting.defaultText;
		} else if (setting.type == MDS_ConfigType::Boolean) {
			defaults[setting.key] = setting.defaultUnsigned != 0;
		} else if (setting.type == MDS_ConfigType::Signed) {
			defaults[setting.key] = setting.defaultSigned;
		} else {
			defaults[setting.key] = setting.defaultUnsigned;
		}

		if (defaults.overflowed()) {
			return fail("Cannot allocate setting defaults");
		}

		if (!valid(setting, defaults[setting.key])) {
			return fail("Invalid setting default");
		}
	}

	values_ = std::move(defaults);
	settings_ = settings;
	settingCount_ = count;
	error_ = "";
	return true;
}

const MDS_ConfigSetting* MDS_Config::settings(size_t& count) const {
	count = settingCount_;
	return settings_;
}

const MDS_ConfigSetting* MDS_Config::find(const char* key) const {
	if (key) {
		for (size_t index = 0; index < settingCount_; ++index) {
			if (strcmp(settings_[index].key, key) == 0) {
				return &settings_[index];
			}
		}
	}

	return nullptr;
}

JsonVariantConst MDS_Config::get(const char* key) const {
	if (!find(key)) {
		return JsonVariantConst();
	}

	return values_[key];
}

bool MDS_Config::set(const char* key, JsonVariantConst value) {
	const auto* setting = find(key);

	if (!setting) {
		return fail("Unknown setting");
	}

	if (setting->access == MDS_ConfigAccess::ReadOnly) {
		return fail("Setting is read-only");
	}

	if (!valid(*setting, value)) {
		return fail("Setting type or range is invalid");
	}

	JsonDocument updated(values_);
	updated[key] = value;

	if (updated.overflowed()) {
		return fail("Cannot allocate setting value");
	}

	values_ = std::move(updated);
	error_ = "";
	return true;
}

bool MDS_Config::loadSettings(const JsonDocument& document) {
	// Validate before applying any field; missing optional fields retain current values.
	for (size_t index = 0; index < settingCount_; ++index) {
		const auto& setting = settings_[index];

		if (!setting.persistent) {
			continue;
		}

		const JsonVariantConst value = document[setting.key];

		if (value.isUnbound()) {
			if (setting.required) {
				return fail("Required setting is missing");
			}
		} else if (!valid(setting, value)) {
			return fail("Stored setting type or range is invalid");
		}
	}

	JsonDocument updated(values_);

	for (size_t index = 0; index < settingCount_; ++index) {
		const auto& setting = settings_[index];

		if (setting.persistent && setting.access == MDS_ConfigAccess::ReadWrite
			&& !document[setting.key].isUnbound()) {
			updated[setting.key] = document[setting.key];
		}
	}

	if (updated.overflowed()) {
		return fail("Cannot allocate configuration values");
	}

	values_ = std::move(updated);
	error_ = "";
	return true;
}

bool MDS_Config::saveSettings(JsonDocument& document) {
	document.to<JsonObject>();

	for (size_t index = 0; index < settingCount_; ++index) {
		const auto& setting = settings_[index];

		if (setting.persistent) {
			document[setting.key] = values_[setting.key];
		}
	}

	if (document.overflowed()) {
		return fail("Cannot allocate configuration document");
	}

	return true;
}

void MDS_Config::dump(Stream& output) const {
	if (!settings_) {
		output.println("Config: no settings registry registered");
		return;
	}

	for (size_t index = 0; index < settingCount_; ++index) {
		const auto& setting = settings_[index];
		output.print("Config: ");
		output.print(setting.key);
		output.print(setting.access == MDS_ConfigAccess::ReadOnly ? " [RO] = " : " [RW] = ");

		if (setting.sensitive) {
			output.print("[redacted]");
		} else {
			serializeJson(values_[setting.key], output);
		}

		output.println();
	}
}

bool MDS_Config::set(const JsonDocument& updates) {
	if (!settings_) {
		return fail("Settings registry not registered");
	}

	if (!updates.is<JsonObjectConst>()) {
		return fail("Setting updates must be an object");
	}

	for (JsonPairConst item : updates.as<JsonObjectConst>()) {
		const auto* setting = find(item.key().c_str());

		if (!setting) {
			return fail("Unknown setting");
		}

		if (setting->access == MDS_ConfigAccess::ReadOnly) {
			return fail("Setting is read-only");
		}

		if (!valid(*setting, item.value())) {
			return fail("Setting type or range is invalid");
		}
	}

	JsonDocument updated(values_);

	for (JsonPairConst item : updates.as<JsonObjectConst>()) {
		updated[item.key()] = item.value();
	}

	if (updated.overflowed()) {
		return fail("Cannot allocate setting values");
	}

	values_ = std::move(updated);
	error_ = "";
	return true;
}

bool MDS_Config::setReadOnly(const char* key, JsonVariantConst value) {
	const auto* setting = find(key);

	if (!setting) {
		return fail("Unknown setting");
	}

	if (setting->access != MDS_ConfigAccess::ReadOnly) {
		return fail("Setting is not read-only");
	}

	if (!valid(*setting, value)) {
		return fail("Setting type or range is invalid");
	}

	JsonDocument updated(values_);
	updated[key] = value;

	if (updated.overflowed()) {
		return fail("Cannot allocate setting value");
	}

	values_ = std::move(updated);
	error_ = "";
	return true;
}
