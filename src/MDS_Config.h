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
	enum class Error {
		None,
		NotInitialized,
		MountFailed,
		FileOpenFailed,
		InvalidDocument,
		WriteFailed,
		InvalidDefinition,
		UnknownSetting,
		AccessDenied,
		InvalidValue,
		CallbackNotSet,
		ModeConflict,
		AllocationFailed
	};

	enum class FormatPolicy {
		Never,
		OnMountFailure
	};

	// Application reads parsed JSON when load() invokes this callback.
	using LoadCallback = std::function<void(JsonDocument&)>;

	// Application fills JSON when save() invokes this callback.
	using SaveCallback = std::function<void(JsonDocument&)>;

	// Select the configuration file; begin() mounts storage separately.
	explicit MDS_Config(const char* path = "/config.json");

	// Mount configuration storage, formatting on failure only when explicitly requested.
	bool begin(FormatPolicy policy = FormatPolicy::Never);

	// Read the saved JSON and apply it through the registry or load callback.
	bool load();

	// Build configuration JSON and replace the saved file after verifying the write.
	bool save();

	// Report whether the mounted filesystem contains the configuration file.
	bool exists() const;

	// Print the saved file verbatim, including any stored secrets.
	void print(Stream& output = Serial);

	// Print current registry values/access flags, redacting sensitive settings.
	void dump(Stream& output = Serial) const;

	// Return the latest failure code without changing configuration or storage.
	Error error() const {
		return error_;
	}

	// Return the latest recorded configuration error, or an empty string when cleared.
	const char* lastError() const {
		return errorText_;
	}

	// Register the callback that load() invokes after parsing the configuration file.
	// Read the supplied JSON into application settings; copy any values needed afterward.
	// Pass nullptr to clear it. Registration is rejected in managed-settings mode.
	void setLoadCallback(LoadCallback callback);

	// Register the callback that save() invokes before writing the configuration file.
	// Fill the supplied JSON from application settings; save() serializes that document.
	// Pass nullptr to clear it. Registration is rejected in managed-settings mode.
	void setSaveCallback(SaveCallback callback);

	// Opt into managed settings; borrow definitions and initialize validated defaults.
	bool setSettings(const MDS_ConfigSetting* settings, size_t count);

	// Register a static array, inferring its number of entries.
	template <size_t Count>
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
	// Record a configuration error and return false to the caller.
	bool fail(Error error, const char* message);

	// Clear the failure code and diagnostic text after a successful operation.
	void clearError();

	// Find setting metadata by key, or return nullptr when the key is unknown.
	const MDS_ConfigSetting* find(const char* key) const;

	// Check the declared type, bounds, and optional application validator.
	static bool valid(const MDS_ConfigSetting& setting, JsonVariantConst value);

	// Validate persistent file values before applying all writable values together.
	bool loadSettings(const JsonDocument& document);

	// Copy persistent registry values into JSON and check for allocation failure.
	bool saveSettings(JsonDocument& document);

	const MDS_ConfigSetting* settings_ = nullptr;
	size_t settingCount_ = 0;
	JsonDocument values_;
	String path_;
	Error error_ = Error::None;
	const char* errorText_ = "";
	bool ready_ = false;
	LoadCallback loadCallback_;
	SaveCallback saveCallback_;
};
