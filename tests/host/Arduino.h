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

#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
using String = std::string;
class Stream {
public:
 virtual ~Stream() = default;
 virtual size_t write(uint8_t) {return 1;}
 size_t write(const uint8_t* data,size_t count){size_t written=0;for(size_t i=0;i<count;++i)written+=write(data[i]);return written;}
 void print(const char* text){while(*text)write(uint8_t(*text++));}
 void println(){write(uint8_t('\n'));}
 void println(const char* text){print(text);println();}
};
extern Stream Serial;
