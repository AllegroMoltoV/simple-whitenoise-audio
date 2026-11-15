// SPDX-License-Identifier: MIT
#pragma once

// Implemented in plugin.cpp and exported by plugin-entry.cpp, as clap-wrapper's clap-first layout expects.
bool swn_entry_init(const char *pluginPath);
void swn_entry_deinit(void);
const void *swn_entry_get_factory(const char *factoryId);
