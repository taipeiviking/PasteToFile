#include "TextWrite.h"

#include <windows.h>

#include "PasteToFileCommon/Filename.h"
#include "PasteToFileCommon/Logging.h"
#include "PasteToFileCommon/Utf.h"

namespace ptf_helper {

static bool TryWriteFile(const std::wstring& path, const void* data, DWORD size) {
  HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                         CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  bool ok = WriteFile(h, data, size, &written, nullptr) != FALSE &&
            written == size;
  CloseHandle(h);
  return ok;
}

static std::wstring NextCandidate(const std::wstring& dir,
                                  const std::wstring& baseName,
                                  const std::wstring& extensionWithDot,
                                  int attempt) {
  if (attempt <= 0) return dir + L"\\" + baseName + extensionWithDot;
  wchar_t suffix[16]{};
  swprintf_s(suffix, L"-%02d", attempt);
  return dir + L"\\" + baseName + suffix + extensionWithDot;
}

bool WriteBinaryFileUniqueWithBase(const std::wstring& targetDir,
                                   const std::wstring& baseName,
                                   const std::wstring& extensionWithDot,
                                   const std::vector<uint8_t>& bytes,
                                   std::wstring* outPath) {
  if (outPath) *outPath = L"";
  for (int attempt = 0; attempt < 1000; attempt++) {
    std::wstring path = NextCandidate(targetDir, baseName, extensionWithDot, attempt);
    if (TryWriteFile(path, bytes.data(), static_cast<DWORD>(bytes.size()))) {
      if (outPath) *outPath = path;
      ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                        L"[Helper] wrote " + path + L" bytes=" +
                            std::to_wstring(bytes.size()));
      return true;
    }
    DWORD err = GetLastError();
    if (err != ERROR_FILE_EXISTS && err != ERROR_ALREADY_EXISTS) {
      ptf::LogLine(L"WriteBinaryFileUnique failed: " + path + L" err=" +
                   std::to_wstring(err));
      ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                        L"[Helper] write failed: " + path + L" err=" +
                            std::to_wstring(err));
      return false;
    }
  }
  return false;
}

bool WriteBinaryFileUnique(const std::wstring& targetDir,
                           const std::wstring& extensionWithDot,
                           const std::vector<uint8_t>& bytes,
                           std::wstring* outPath) {
  return WriteBinaryFileUniqueWithBase(targetDir, ptf::BuildDatedBaseName(),
                                       extensionWithDot, bytes, outPath);
}

bool WriteUtf8TextFileUniqueWithBase(const std::wstring& targetDir,
                                     const std::wstring& baseName,
                                     const std::wstring& extensionWithDot,
                                     const std::wstring& text,
                                     std::wstring* outPath) {
  std::string utf8 = ptf::WideToUtf8(text);
  std::vector<uint8_t> bytes(utf8.begin(), utf8.end());
  return WriteBinaryFileUniqueWithBase(targetDir, baseName, extensionWithDot, bytes,
                                       outPath);
}

bool WriteUtf8TextFileUnique(const std::wstring& targetDir,
                             const std::wstring& extensionWithDot,
                             const std::wstring& text,
                             std::wstring* outPath) {
  return WriteUtf8TextFileUniqueWithBase(targetDir, ptf::BuildDatedBaseName(),
                                         extensionWithDot, text, outPath);
}

} // namespace ptf_helper
