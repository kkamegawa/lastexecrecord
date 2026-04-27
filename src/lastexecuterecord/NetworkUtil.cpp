// NetworkUtil.cpp
#include "NetworkUtil.h"

#include <Windows.h>
#include <combaseapi.h>
#include <netlistmgr.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ler {
namespace {

struct NetworkProbe {
    bool statusKnown = false;
    bool hasInternet = false;
    bool isMetered = false;
    HRESULT error = S_OK;
};

static bool hasInternetConnectivity(NLM_CONNECTIVITY connectivity) {
    return (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
        (connectivity & NLM_CONNECTIVITY_IPV6_INTERNET);
}

static NetworkProbe probeNetwork(bool requireCost) {
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitialized = SUCCEEDED(hrInit);

    NetworkProbe probe;
    {
        ComPtr<INetworkListManager> networkListManager;
        HRESULT hr = CoCreateInstance(
            CLSID_NetworkListManager,
            nullptr,
            CLSCTX_ALL,
            IID_INetworkListManager,
            reinterpret_cast<void**>(networkListManager.GetAddressOf()));

        if (FAILED(hr)) {
            probe.error = hr;
        }
        else {
            NLM_CONNECTIVITY connectivity{};
            hr = networkListManager->GetConnectivity(&connectivity);
            if (FAILED(hr)) {
                probe.error = hr;
            }
            else {
                probe.hasInternet = hasInternetConnectivity(connectivity);
                probe.statusKnown = true;

                if (probe.hasInternet && requireCost) {
                    ComPtr<INetworkCostManager> costManager;
                    hr = networkListManager->QueryInterface(
                        IID_INetworkCostManager,
                        reinterpret_cast<void**>(costManager.GetAddressOf()));
                    if (FAILED(hr)) {
                        probe.statusKnown = false;
                        probe.error = hr;
                    }
                    else {
                        DWORD costFlags = 0;
                        hr = costManager->GetCost(&costFlags, nullptr);
                        if (FAILED(hr)) {
                            probe.statusKnown = false;
                            probe.error = hr;
                        }
                        else {
                            probe.isMetered = (costFlags != NLM_CONNECTION_COST_UNRESTRICTED);
                        }
                    }
                }
            }
        }
    }

    if (comInitialized) {
        CoUninitialize();
    }

    return probe;
}

} // namespace

bool hasInternetConnection() {
    NetworkProbe probe = probeNetwork(false);
    return probe.statusKnown && probe.hasInternet;
}

bool isConnectionMetered() {
    NetworkProbe probe = probeNetwork(true);
    return probe.statusKnown && probe.isMetered;
}

NetworkDecision evaluateNetworkOption(NetworkOption option) {
    if (option == NetworkOption::AlwaysExecute) {
        return NetworkDecision{ true, true, 0 };
    }

    if (option != NetworkOption::ExecuteOnMetered &&
        option != NetworkOption::ExecuteWhenConnected) {
        return NetworkDecision{ false, false, ERROR_INVALID_PARAMETER };
    }

    NetworkProbe probe = probeNetwork(option == NetworkOption::ExecuteWhenConnected);
    if (!probe.statusKnown) {
        return NetworkDecision{ false, false, static_cast<std::uint32_t>(probe.error) };
    }

    if (option == NetworkOption::ExecuteOnMetered) {
        return NetworkDecision{ probe.hasInternet, true, 0 };
    }

    return NetworkDecision{ probe.hasInternet && !probe.isMetered, true, 0 };
}

bool shouldExecuteBasedOnNetwork(NetworkOption option) {
    return evaluateNetworkOption(option).shouldExecute;
}

} // namespace ler
