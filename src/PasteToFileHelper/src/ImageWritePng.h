#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace ptf_helper {

bool WritePngFileUniqueFromHbitmap(const std::wstring& targetDir,
                                   HBITMAP hbm,
                                   std::wstring* outPath);

// Decodes the provided encoded image bytes (png/jpg/gif/...) via WIC and writes a PNG.
// Uses an explicit base file name (without extension).
bool WritePngFileUniqueFromEncodedImageBytesWithBase(const std::wstring& targetDir,
                                                     const std::wstring& baseName,
                                                     const std::vector<uint8_t>& bytes,
                                                     std::wstring* outPath);

} // namespace ptf_helper
