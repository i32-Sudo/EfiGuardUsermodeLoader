#include "disk.h"

void diskUtil::ExecuteDiskpartScript(const std::string& partitionName, int diskNumber) {
    std::string diskpartScript = R"(
select disk )" + std::to_string(diskNumber) + R"(
create partition primary size=30
format fs=ntfs quick label=)" + partitionName + R"(
assign letter=)" + partitionName + "\n";

    std::ofstream script("create_partition.txt");
    script << diskpartScript;
    script.close();

    systemUtil::ExecuteCommand("diskpart /s create_partition.txt");
    DeleteFile("create_partition.txt");
}

void diskUtil::ShrinkPartition(const std::string& driveLetter, int shrinkSizeMB) {
    std::string diskpartScript = R"(
select volume )" + driveLetter + R"(
shrink desired=)" + std::to_string(shrinkSizeMB) + "\n";

    std::ofstream script("shrink_partition.txt");
    script << diskpartScript;
    script.close();

    systemUtil::ExecuteCommand("diskpart /s shrink_partition.txt");
    DeleteFile("shrink_partition.txt");
}

bool diskUtil::PartitionExists(const std::string& partitionName) {
    std::ofstream script("list_partitions.txt");
    script << "list volume\n";
    script.close();

    std::string output = systemUtil::GetCommandOutput("diskpart /s list_partitions.txt");
    DeleteFile("list_partitions.txt");

    return output.find(partitionName) != std::string::npos;
}

void diskUtil::DeletePartition(const std::string& partitionName) {
    std::ofstream script("delete_partition.txt");
    script << "select volume " << partitionName << "\n";
    script << "delete volume\n";
    script.close();

    systemUtil::ExecuteCommand("diskpart /s delete_partition.txt");
    DeleteFile("delete_partition.txt");
}


int diskUtil::CreatePartition(const std::string& partitionName) {
    int validDiskNum = 0;
    for (int diskNumber = 0; diskNumber < 4; ++diskNumber) {
        diskUtil::ExecuteDiskpartScript(partitionName, diskNumber);
        if (diskUtil::PartitionExists("X")) {
            validDiskNum = diskNumber;
            break;
        }
    }
    return validDiskNum;
}

void diskUtil::CreateDirectoryPath(const std::string& directoryPath) {
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

unsigned long long diskUtil::getPartitionSizeInMB(const std::string& drive) {
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    std::string drivePath = drive;
    if (drive.back() != '\\') {
        drivePath += '\\';
    }

    if (GetDiskFreeSpaceEx(drivePath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes) == 0) {
        //std::cerr << "Error: Unable to get disk space information for " << drive << std::endl;
        return 0;
    }
    unsigned long long totalSizeInMB = totalBytes.QuadPart / (1024 * 1024);

    return totalSizeInMB;
}

bool diskUtil::writeFileBytes(const std::string& filePath, unsigned char* data, size_t dataSize) {
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
    if (outFile) {
        outFile.write(reinterpret_cast<const char*>(data), dataSize);
        outFile.close();
        return true;
    }
    else {
        // std::cerr << "Error opening file " << filePath << " for writing.\n";
        return false;
    }
}