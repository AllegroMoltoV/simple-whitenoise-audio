// SPDX-License-Identifier: MIT
// clap-wrapper compiles this file into both the CLAP and the VST 3 module.

#include <clap/clap.h>

#include "plugin-entry.h"

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, swn_entry_init, swn_entry_deinit,
                                                               swn_entry_get_factory};
