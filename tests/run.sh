#!/bin/sh
# ===========================================================================
# MDS_Config 0.7.0 - https://home.marcspencer.com/mds-config
#
# Copyright (c) 2026 Marc D. Spencer.
# All Rights Reserved under the applicable license.
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
# ===========================================================================

set -eu
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
LIBRARY_DIR=$(CDPATH= cd -- "$TEST_DIR/.." && pwd)
: "${ARDUINOJSON_SRC:?Set ARDUINOJSON_SRC to the ArduinoJson 7 src directory}"
TEST_BUILD=$(mktemp -d)
trap 'rm -rf "$TEST_BUILD"' EXIT HUP INT TERM
for test in legacy registry dump batch readonly validator signed errors; do
 c++ -std=c++17 -Wall -Wextra -I"$TEST_DIR/host" -I"$LIBRARY_DIR/src" -I"$ARDUINOJSON_SRC" "$TEST_DIR/$test.cpp" "$LIBRARY_DIR/src/MDS_Config.cpp" -o "$TEST_BUILD/$test"
 "$TEST_BUILD/$test"
done
