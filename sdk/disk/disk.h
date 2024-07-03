#pragma once
#include "../system/system.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>


namespace diskUtil {
	void ExecuteDiskpartScript(const std::string& partitionName, int diskNumber);

	void ShrinkPartition(const std::string& driveLetter, int shrinkSizeMB);

	bool PartitionExists(const std::string& partitionName);

	void DeletePartition(const std::string& partitionName);

	int CreatePartition(const std::string& partitionName);

	void CreateDirectoryPath(const std::string& directoryPath);

	unsigned long long getPartitionSizeInMB(const std::string& drive);

	bool writeFileBytes(const std::string& filePath, unsigned char* data, size_t dataSize);
}