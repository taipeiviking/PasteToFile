#pragma once

#include <string>
#include <vector>

namespace ptf_helper {

bool WriteUtf8TextFileUnique(const std::wstring& targetDir,
                             const std::wstring& extensionWithDot,
                             const std::wstring& text,
                             std::wstring* outPath);

bool WriteBinaryFileUnique(const std::wstring& targetDir,
                           const std::wstring& extensionWithDot,
                           const std::vector<uint8_t>& bytes,
                           std::wstring* outPath);

// Same as above, but uses an explicit base file name (without extension).
// Example baseName: "PTF-2026-feb-23-HIST-0001"
bool WriteUtf8TextFileUniqueWithBase(const std::wstring& targetDir,
                                     const std::wstring& baseName,
                                     const std::wstring& extensionWithDot,
                                     const std::wstring& text,
                                     std::wstring* outPath);

bool WriteBinaryFileUniqueWithBase(const std::wstring& targetDir,
                                   const std::wstring& baseName,
                                   const std::wstring& extensionWithDot,
                                   const std::vector<uint8_t>& bytes,
                                   std::wstring* outPath);

} // namespace ptf_helper
