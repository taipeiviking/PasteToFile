#pragma once

#include <string>

namespace ptf {

// Returns %LOCALAPPDATA%\\PasteToFile (creates it if missing).
std::wstring GetAppDataDir();

std::wstring JoinPath(const std::wstring& a, const std::wstring& b);

bool EnsureDirectoryExists(const std::wstring& path);

} // namespace ptf
