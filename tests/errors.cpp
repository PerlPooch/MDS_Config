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
#include <utility>

using Error = MDS_Config::Error;
using Type = MDS_ConfigType;
using Access = MDS_ConfigAccess;
Stream Serial;
FakeFS LittleFS;
const MDS_ConfigSetting definitions[] = {
	{"schema", Type::Unsigned, Access::ReadOnly, true, nullptr, 1, 1, 1, true},
	{"rate", Type::Unsigned, Access::ReadWrite, true, nullptr, 5, 0, 100}};

// Reject JSON allocations so save() can classify an overflowed callback document.
class DenyAllocator : public ArduinoJson::Allocator {
public:
	// Simulate an exhausted heap on the first allocation.
	void* allocate(size_t) override {
		return nullptr;
	}

	// Simulate failure to resize a JSON allocation.
	void* reallocate(void*, size_t) override {
		return nullptr;
	}

	// No allocation ever succeeds, so there is nothing to release.
	void deallocate(void*) override {
	}
};

// Assert that successful operations clear both parts of the diagnostic state.
static void expectClear(const MDS_Config& config) {
	assert(config.error() == Error::None);
	assert(std::strlen(config.lastError()) == 0);
}

// Exercise typed failures without changing persistence, validation or recovery behavior.
int main() {
	MDS_Config config;
	expectClear(config);
	assert(!config.load() && config.error() == Error::NotInitialized);
	assert(!config.save() && config.error() == Error::NotInitialized);
	LittleFS.mountOK = false;
	assert(!config.begin() && config.error() == Error::MountFailed);
	assert(std::strcmp(config.lastError(), "LittleFS mount failed") == 0);
	assert(!config.exists());
	assert(config.error() == Error::MountFailed);
	LittleFS.mountOK = true;
	assert(config.begin());
	expectClear(config);
	assert(!config.load() && config.error() == Error::CallbackNotSet);
	config.setLoadCallback([](JsonDocument&) {
		// Parsing is sufficient for these callback-mode error checks.
	});
	assert(config.error() == Error::CallbackNotSet);
	assert(!config.load() && config.error() == Error::FileOpenFailed);
	LittleFS.files["/config.json"] = "{";
	assert(!config.load() && config.error() == Error::InvalidDocument);
	LittleFS.files["/config.json"] = "[]";
	assert(!config.load() && config.error() == Error::InvalidDocument);
	LittleFS.files["/config.json"] = "{}";
	assert(config.load());
	expectClear(config);
	assert(!config.save() && config.error() == Error::CallbackNotSet);
	config.setSaveCallback([](JsonDocument& document) {
		// Leave an invalid root type for save() to reject.
		document.to<JsonArray>();
	});
	assert(!config.save() && config.error() == Error::InvalidDocument);

	DenyAllocator allocator;
	config.setSaveCallback([&](JsonDocument& document) {
		// Transfer an overflowed document without requiring heap exhaustion in the host.
		JsonDocument failed(&allocator);
		failed["value"] = 1;
		assert(failed.overflowed());
		document = std::move(failed);
	});
	assert(!config.save() && config.error() == Error::AllocationFailed);
	config.setSaveCallback([](JsonDocument& document) {
		// Supply a normal object for persistence fault injection.
		document["rate"] = 5;
	});
	LittleFS.writeOK = false;
	assert(!config.save() && config.error() == Error::WriteFailed);
	LittleFS.writeOK = true;
	LittleFS.shortWrite = true;
	assert(!config.save() && config.error() == Error::WriteFailed);
	assert(!LittleFS.exists("/config.json.tmp"));
	LittleFS.shortWrite = false;
	LittleFS.readOK = false;
	assert(!config.save() && config.error() == Error::WriteFailed);
	LittleFS.readOK = true;
	LittleFS.renameOK = false;
	assert(!config.save() && config.error() == Error::WriteFailed);
	LittleFS.renameOK = true;
	assert(config.save());
	expectClear(config);

	assert(!config.setSettings(definitions) && config.error() == Error::ModeConflict);
	config.setLoadCallback(nullptr);
	config.setSaveCallback(nullptr);
	assert(config.error() == Error::ModeConflict);
	assert(!config.setSettings(nullptr, 0) && config.error() == Error::InvalidDefinition);
	auto invalid = definitions[0];
	invalid.defaultUnsigned = 2;
	assert(!config.setSettings(&invalid, 1) && config.error() == Error::InvalidDefinition);
	MDS_ConfigSetting duplicates[] = {definitions[0], definitions[0]};
	assert(!config.setSettings(duplicates) && config.error() == Error::InvalidDefinition);
	assert(config.setSettings(definitions));
	expectClear(config);
	config.setLoadCallback(nullptr);
	assert(config.error() == Error::ModeConflict);
	config.setSaveCallback(nullptr);
	assert(config.error() == Error::ModeConflict);

	JsonDocument input;
	input["value"] = 10;
	assert(!config.set("missing", input["value"]) && config.error() == Error::UnknownSetting);
	assert(!config.set("schema", input["value"]) && config.error() == Error::AccessDenied);
	assert(!config.setReadOnly("rate", input["value"]) && config.error() == Error::AccessDenied);
	input["value"] = 101;
	assert(!config.set("rate", input["value"]) && config.error() == Error::InvalidValue);
	assert(config.get("rate").as<int>() == 5);
	assert(config.get("missing").isNull());
	size_t count = 0;
	assert(config.settings(count) == definitions && count == 2);
	config.dump();
	config.print();
	assert(config.error() == Error::InvalidValue);
	input["value"] = 0;
	assert(config.set("rate", input["value"]));
	expectClear(config);
	assert(config.save());
	assert(config.load());
	assert(config.get("rate").as<int>() == 0);
	expectClear(config);

	LittleFS.files["/config.json"] = "{}";
	assert(!config.load() && config.error() == Error::InvalidValue);
	LittleFS.files["/config.json"] = "{\"schema\":1,\"rate\":101}";
	assert(!config.load() && config.error() == Error::InvalidValue);
	JsonDocument batch;
	batch.to<JsonArray>();
	assert(!config.set(batch) && config.error() == Error::InvalidDocument);
	batch.to<JsonObject>();
	batch["missing"] = 1;
	assert(!config.set(batch) && config.error() == Error::UnknownSetting);
	batch.clear();
	batch["schema"] = 1;
	assert(!config.set(batch) && config.error() == Error::AccessDenied);
	batch.clear();
	batch["rate"] = 200;
	assert(!config.set(batch) && config.error() == Error::InvalidValue);
	batch["rate"] = 11;
	assert(config.set(batch));
	expectClear(config);
	assert(!config.setReadOnly("missing", input["value"]));
	assert(config.error() == Error::UnknownSetting);
	assert(!config.setReadOnly("schema", input["value"]));
	assert(config.error() == Error::InvalidValue);
	input["value"] = 1;
	assert(config.setReadOnly("schema", input["value"]));
	expectClear(config);

	MDS_Config unregistered;
	assert(!unregistered.set(batch));
	assert(unregistered.error() == Error::NotInitialized);
	puts("PASS: Config typed errors, callback/registry failures, write faults and clearing rules.");
}
