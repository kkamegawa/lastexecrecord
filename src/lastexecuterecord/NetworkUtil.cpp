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
        // Get all network connections
        ComPtr<IEnumNetworkConnections> pEnumConnections;
        hr = pNetworkListManager->GetNetworkConnections(pEnumConnections.GetAddressOf());
        
        if (SUCCEEDED(hr)) {
            ComPtr<INetworkConnection> pConnection;
            ULONG fetched = 0;
            
            // Check each connection
            while (pEnumConnections->Next(1, pConnection.GetAddressOf(), &fetched) == S_OK && fetched > 0) {
                VARIANT_BOOL isConnected = VARIANT_FALSE;
                pConnection->get_IsConnected(&isConnected);
                
                if (isConnected == VARIANT_TRUE) {
                    // Get the network associated with this connection
                    ComPtr<INetwork> pNetwork;
                    hr = pConnection->GetNetwork(pNetwork.GetAddressOf());
                    
                    if (SUCCEEDED(hr)) {
                        // On Windows 8+, we can use INetworkCostManager
                        ComPtr<INetworkCostManager> pCostManager;
                        hr = CoCreateInstance(
                            CLSID_NetworkListManager,
                            nullptr,
                            CLSCTX_ALL,
                            IID_INetworkCostManager,
                            reinterpret_cast<void**>(pCostManager.GetAddressOf())
                        );
                        
                        if (SUCCEEDED(hr)) {
                            DWORD costFlags = 0;
                            hr = pCostManager->GetCost(&costFlags, nullptr);
                            
                            if (SUCCEEDED(hr)) {
                                // Check if connection is NOT unrestricted (meaning it's metered)
                                if (costFlags != NLM_CONNECTION_COST_UNRESTRICTED) {
                                    isMetered = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                pConnection.Reset();
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
            
        case NetworkOption::ExecuteWhenConnected:
            // Execute only if connected and NOT metered
            if (!hasInternetConnection()) {
                return false;
            }
            return !isConnectionMetered();
            
        default:
            // Unknown option, default to always execute
            return true;
    }
}

} // namespace ler
