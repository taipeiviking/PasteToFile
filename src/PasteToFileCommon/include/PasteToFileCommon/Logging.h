#pragma once

#include <windows.h>

#include <string>

namespace ptf {

// Appends a timestamped line to %LOCALAPPDATA%\\PasteToFile\\ptf.log
void LogLine(const std::wstring& line);

// Writes a timestamped line to a debug log file. Preferred location:
// - If PTF_LOG_DIR is set: %PTF_LOG_DIR%\\<fileName>
// - Else: <moduleDir>\\<fileName> (if writable)
// - Else: %LOCALAPPDATA%\\PasteToFile\\<fileName>
void LogLineDebug(HMODULE moduleForDir, const std::wstring& fileName,
                  const std::wstring& line);

} // namespace ptf
