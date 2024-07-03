#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

namespace systemUtil {
	void ExecuteCommand(const std::string& command);

	std::string GetCommandOutput(const std::string& command);
	bool RunAsAdmin(const std::string& exePath, const std::string& arguments);

	void CreateDirectoryPath(const std::string& directoryPath);
	void CopyFiles(const std::vector<std::pair<std::string, std::string>>& files);

	void ForceRestart();
	std::string GetTempDirectory();

	// я знаю что эти функции отсталые смирись ублюдок.
	bool IsSecureBootEnabled();
	bool IsTPMEnabled();

	int GetWindowsVersion();
}