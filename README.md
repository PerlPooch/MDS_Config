# MDS_Config

JSON configuration storage and typed settings with validation and LittleFS persistence.

## Requirements

- **Platforms:** ESP32.
- **Dependencies:** ArduinoJson 7 or later.

## Installation

Place the `MDS_Config` folder in your Arduino sketchbook's `libraries` directory. Install the required board package and dependencies using the Arduino IDE.

## Example

Give an imaginary coffee maker a name, a cup count, and an on/off setting. The settings table defines types, defaults, limits, access, and persistence. MDS_Config permits ephemeral and persistent configuration information. The model is read-only and is not saved; the other settings survive a restart.
Install **ArduinoJson 7 or later** along with **MDS_Config**. The ESP32 must have a formatted LittleFS filesystem; this example does not format it automatically.

```cpp
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

    if (!config.setSettings(settings) || !config.begin()) {
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
```

## Documentation

Detailed design and API documentation: [MDS_Config](https://home.marcspencer.com/mds-config).

## License

Firmware and software: **MPL-2.0**. Documentation and website: **CC BY-SA 4.0**. Any hardware design files and Gerbers: **CERN-OHL-W-2.0**.

## Contributions

Contributions require a Developer Certificate of Origin (DCO) sign-off. Use `git commit -s` to add a `Signed-off-by` line to each contribution commit.

## Initialization API

`begin()` is the initialization method; the former `setup()` name is removed.
The default is `MDS_Config::FormatPolicy::Never`. To allow filesystem formatting
after a failed mount, pass `MDS_Config::FormatPolicy::OnMountFailure`.

## Configuration callbacks

Callback mode keeps the settings in the application. Register callbacks before
calling `load()` or `save()`:

```cpp
// Copy parsed file contents into application-owned settings during load().
config.setLoadCallback([](JsonDocument& document) {
    applicationName = document["name"] | "";
});

// Fill the JSON document that save() will serialize to the configuration file.
config.setSaveCallback([](JsonDocument& document) {
    document["name"] = applicationName;
});
```

Here `applicationName` is an application-owned `String`. Registration does not
load or save anything. The callback receives a temporary document, so copy values
needed after it returns. Passing `nullptr` clears a callback. Managed-settings
mode uses its registry directly and rejects callback registration.

The former `setSetConfigCallback()` and `setGetConfigCallback()` names are removed;
use `setLoadCallback()` and `setSaveCallback()`, respectively. Callback signatures
and load/save behavior are unchanged.

Callback function types are named `MDS_Config::LoadCallback` and
`MDS_Config::SaveCallback`, matching their registration methods. Both remain
`std::function<void(JsonDocument&)>`; the former generic `Callback` alias is removed.

### Error reporting

`error()` returns a class-scoped `Error` code; `lastError()` supplies diagnostic
text. Codes cover initialization, mounting, opening a saved file, invalid JSON,
writing, setting definitions, unknown keys, access, values, missing callbacks,
mode conflicts and allocation failures. `Error::None` has empty diagnostic text.

Successful `begin()`, `load()`, `save()`, `setSettings()`, `set()` and
`setReadOnly()` clear prior errors. Queries, printing and successful callback
registration leave them unchanged. Rejected callback registration records
`ModeConflict`. Missing required settings and invalid stored setting values use
`InvalidValue`; malformed or non-object JSON uses `InvalidDocument`. Temporary
file open, write verification and replacement failures use `WriteFailed`.
