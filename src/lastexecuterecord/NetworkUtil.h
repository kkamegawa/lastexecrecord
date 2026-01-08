// NetworkUtil.h
#pragma once

#include <cstdint>

namespace ler {

// Network option values for controlling execution based on connectivity
enum class NetworkOption : std::int32_t {
    ExecuteWhenConnected = 0,    // Execute only when internet is connected (not metered)
    ExecuteOnMetered = 1,         // Execute even on metered connections
    AlwaysExecute = 2             // Always execute (ignore network status)
};

// Check if the system has internet connectivity
// Returns true if connected to internet (any type)
bool hasInternetConnection();

// Check if the current connection is metered
// Returns true if the connection is metered, false otherwise
// If no connection exists, returns false
bool isConnectionMetered();

// Check if execution should proceed based on network option
// Returns true if execution should continue, false if it should be skipped
bool shouldExecuteBasedOnNetwork(NetworkOption option);

} // namespace ler
