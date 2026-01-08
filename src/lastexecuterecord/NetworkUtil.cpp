// NetworkUtil.cpp
#include "NetworkUtil.h"

#include <Windows.h>
#include <netlistmgr.h>
#include <combaseapi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ler {

bool hasInternetConnection() {
    // Initialize COM for this thread if not already initialized
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitialized = (hrInit == S_OK);
    
    // Create NetworkListManager instance
    ComPtr<INetworkListManager> pNetworkListManager;
    HRESULT hr = CoCreateInstance(
        CLSID_NetworkListManager,
        nullptr,
        CLSCTX_ALL,
        IID_INetworkListManager,
        reinterpret_cast<void**>(pNetworkListManager.GetAddressOf())
    );
    
    bool hasConnection = false;
    if (SUCCEEDED(hr)) {
        NLM_CONNECTIVITY connectivity;
        hr = pNetworkListManager->GetConnectivity(&connectivity);
        if (SUCCEEDED(hr)) {
            // Check if we have internet connectivity
            hasConnection = (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
                           (connectivity & NLM_CONNECTIVITY_IPV6_INTERNET);
        }
    }
    
    if (comInitialized) {
        CoUninitialize();
    }
    
    return hasConnection;
}

bool isConnectionMetered() {
    // Initialize COM for this thread if not already initialized
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitialized = (hrInit == S_OK);
    
    // Create NetworkListManager instance
    ComPtr<INetworkListManager> pNetworkListManager;
    HRESULT hr = CoCreateInstance(
        CLSID_NetworkListManager,
        nullptr,
        CLSCTX_ALL,
        IID_INetworkListManager,
        reinterpret_cast<void**>(pNetworkListManager.GetAddressOf())
    );
    
    bool isMetered = false;
    if (SUCCEEDED(hr)) {
        // Use INetworkCostManager to check cost
        ComPtr<INetworkCostManager> pCostManager;
        hr = pNetworkListManager->QueryInterface(IID_INetworkCostManager,
                                                 reinterpret_cast<void**>(pCostManager.GetAddressOf()));
        
        if (SUCCEEDED(hr)) {
            DWORD costFlags = 0;
            hr = pCostManager->GetCost(&costFlags, nullptr);
            
            if (SUCCEEDED(hr)) {
                // Check if connection is NOT unrestricted (meaning it's metered)
                isMetered = (costFlags != NLM_CONNECTION_COST_UNRESTRICTED);
            }
        }
    }
    
    if (comInitialized) {
        CoUninitialize();
    }
    
    return isMetered;
}

bool shouldExecuteBasedOnNetwork(NetworkOption option) {
    switch (option) {
        case NetworkOption::AlwaysExecute:
            // Always execute regardless of network status
            return true;
            
        case NetworkOption::ExecuteOnMetered:
            // Execute if any internet connection exists (metered or not)
            return hasInternetConnection();
            
        case NetworkOption::ExecuteWhenConnected: {
            // Execute only if connected and NOT metered
            // Optimize by checking connection and metered status together
            // Initialize COM for this thread if not already initialized
            HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            bool comInitialized = (hrInit == S_OK);
            
            // Create NetworkListManager instance
            ComPtr<INetworkListManager> pNetworkListManager;
            HRESULT hr = CoCreateInstance(
                CLSID_NetworkListManager,
                nullptr,
                CLSCTX_ALL,
                IID_INetworkListManager,
                reinterpret_cast<void**>(pNetworkListManager.GetAddressOf())
            );
            
            bool shouldExecute = false;
            if (SUCCEEDED(hr)) {
                // Check connectivity first
                NLM_CONNECTIVITY connectivity;
                hr = pNetworkListManager->GetConnectivity(&connectivity);
                if (SUCCEEDED(hr)) {
                    bool hasConnection = (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
                                        (connectivity & NLM_CONNECTIVITY_IPV6_INTERNET);
                    
                    if (hasConnection) {
                        // Connected - now check if metered using INetworkCostManager
                        ComPtr<INetworkCostManager> pCostManager;
                        hr = pNetworkListManager->QueryInterface(IID_INetworkCostManager, 
                                                                 reinterpret_cast<void**>(pCostManager.GetAddressOf()));
                        
                        if (SUCCEEDED(hr)) {
                            DWORD costFlags = 0;
                            hr = pCostManager->GetCost(&costFlags, nullptr);
                            
                            if (SUCCEEDED(hr)) {
                                // Execute only if NOT metered
                                shouldExecute = (costFlags == NLM_CONNECTION_COST_UNRESTRICTED);
                            }
                            else {
                                // If cost check fails, assume unrestricted for safety
                                shouldExecute = true;
                            }
                        }
                        else {
                            // If cost manager not available, assume unrestricted
                            shouldExecute = true;
                        }
                    }
                }
            }
            
            if (comInitialized) {
                CoUninitialize();
            }
            
            return shouldExecute;
        }
            
        default:
            // Unknown option, default to always execute
            return true;
    }
}

} // namespace ler
