#pragma warning(disable: 4996)

#include "system.h"

void systemUtil::ExecuteCommand(const std::string& command) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        //std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
    }
}

std::string systemUtil::GetCommandOutput(const std::string& command) {
    std::string output;
    char buffer[128];
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            output += buffer;
        }
    }
    catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    return output;
}


bool systemUtil::RunAsAdmin(const std::string& exePath, const std::string& arguments) {
    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = "runas";  // Request administrator privileges
    sei.lpFile = exePath.c_str();  // The executable to run
    sei.lpParameters = arguments.c_str();  // The arguments to pass to the executable
    sei.nShow = SW_SHOWNORMAL;  // Show the window normally

    if (!ShellExecuteEx(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            // std::cerr << "The user refused to allow privileges elevation." << std::endl;
        }
        else {
            // std::cerr << "Error: " << error << std::endl;
        }
        return false;
    }

    return true;
}

int systemUtil::GetWindowsVersion() {
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        if (osvi.dwMajorVersion == 10) {
            if (osvi.dwBuildNumber >= 22000) {
                return 11; // Windows 11 starts from build 22000
            }
            else {
                return 10;
            }
        }
    }

    return 0;
}

void systemUtil::CreateDirectoryPath(const std::string & directoryPath) {
    size_t pos = 0;
    std::string path;
    while ((pos = directoryPath.find('\\', pos)) != std::string::npos) {
        path = directoryPath.substr(0, pos++);
        if (!path.empty() && !CreateDirectory(path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            // std::cerr << "CreateDirectory failed (" << GetLastError() << "): " << path << std::endl;
        }
    }
    if (!CreateDirectory(directoryPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        // std::cerr << "CreateDirectory failed (" << GetLastError() << "): " << directoryPath << std::endl;
    }
}

void systemUtil::CopyFiles(const std::vector<std::pair<std::string, std::string>>&files) {
    for (const auto& file : files) {
        size_t lastSlash = file.second.find_last_of('\\');
        if (lastSlash != std::string::npos) {
            std::string dirPath = file.second.substr(0, lastSlash);
            CreateDirectoryPath(dirPath);
        }

        if (!CopyFile(file.first.c_str(), file.second.c_str(), FALSE)) {
            // std::cerr << "Failed to copy file " << file.first << " to " << file.second << ". Error: " << GetLastError() << std::endl;
        }
    }
}

void systemUtil::ForceRestart() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        // std::cerr << "OpenProcessToken failed: " << GetLastError() << "\n";
        return;
    }

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS) {
        // std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << "\n";
        return;
    }

    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED)) {
        // std::cerr << "ExitWindowsEx failed: " << GetLastError() << "\n";
    }
}

std::string systemUtil::GetTempDirectory() {
    const char* tempDir = std::getenv("TMPDIR");
    if (!tempDir) {
        tempDir = std::getenv("TMP");
    }
    if (!tempDir) {
        tempDir = std::getenv("TEMP");
    }
    if (!tempDir) {
        tempDir = "/tmp";
    }
    return std::string(tempDir);
}

bool systemUtil::IsSecureBootEnabled() {
    IWbemLocator* pLocator = nullptr;
    IWbemServices* pServices = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject* pClassObject = nullptr;
    ULONG uReturn = 0;
    bool isSecureBootEnabled = false;

    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
        return false;
    if (FAILED(CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr)))
        return false;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator)))
        return false;
    if (FAILED(pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2\\Security\\MicrosoftTpm"), nullptr, nullptr, 0, NULL, 0, 0, &pServices)))
        return false;
    if (FAILED(CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE)))
        return false;
    if (FAILED(pServices->ExecQuery(bstr_t("WQL"), bstr_t("SELECT SecureBoot FROM Win32_BIOS"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator)))
        return false;

    while (pEnumerator) {
        if (pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn) == S_OK) {
            VARIANT vtProp;
            pClassObject->Get(L"SecureBoot", 0, &vtProp, 0, 0);
            if (vtProp.vt == VT_BOOL)
                isSecureBootEnabled = vtProp.boolVal == VARIANT_TRUE;
            VariantClear(&vtProp);
            pClassObject->Release();
        }
        break;
    }

    if (pLocator) pLocator->Release();
    if (pServices) pServices->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
    return isSecureBootEnabled;
}

bool systemUtil::IsTPMEnabled() {
    IWbemLocator* pLocator = nullptr;
    IWbemServices* pServices = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject* pClassObject = nullptr;
    ULONG uReturn = 0;
    bool isTPMEnabled = false;

    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
        return false;
    if (FAILED(CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr)))
        return false;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator)))
        return false;
    if (FAILED(pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2\\Security\\MicrosoftTpm"), nullptr, nullptr, 0, NULL, 0, 0, &pServices)))
        return false;
    if (FAILED(CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE)))
        return false;
    if (FAILED(pServices->ExecQuery(bstr_t("WQL"), bstr_t("SELECT IsActivated_InitialValue FROM Win32_Tpm"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator)))
        return false;

    while (pEnumerator) {
        if (pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn) == S_OK) {
            VARIANT vtProp;
            pClassObject->Get(L"IsActivated_InitialValue", 0, &vtProp, 0, 0);
            if (vtProp.vt == VT_BOOL)
                isTPMEnabled = vtProp.boolVal == VARIANT_TRUE;
            VariantClear(&vtProp);
            pClassObject->Release();
        }
        break;
    }

    if (pLocator) pLocator->Release();
    if (pServices) pServices->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
    return isTPMEnabled;
}

