// Single source of truth for firmware versioning.
#pragma once

#define FW_MAJOR 2
#define FW_MINOR 5
#define FW_PATCH 0
#define FW_CHANNEL "MAIN"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// e.g., "Starting V2.1.0 MAIN"
#define FW_VERSION_STR "Starting V" STR(FW_MAJOR) "." STR(FW_MINOR) "." STR(FW_PATCH) " " FW_CHANNEL
