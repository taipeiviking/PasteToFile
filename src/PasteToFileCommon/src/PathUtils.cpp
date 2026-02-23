#include "PasteToFileCommon/PathUtils.h"

#include <windows.h>
#include <shlobj.h>

namespace ptf {

static std::wstring TrimTrailingBackslashes(std::wstring s) {
  while (!s.empty() && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
  return s;
}

std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
  std::wstring aa = TrimTrailingBackslashes(a);
  if (b.front() == L'\\' || b.front() == L'/') return aa + b;
  return aa + L"\\" + b;
}

bool EnsureDirectoryExists(const std::wstring& path) {
  if (path.empty()) return false;

  DWORD attrs = GetFileAttributesW(path.c_str());
  if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
    return true;
  }

  // CreateDirectoryW only creates one level; do a simple recursive create.
  size_t pos = 0;
  while (true) {
    pos = path.find_first_of(L"\\/", pos + 1);
    std::wstring sub = (pos == std::wstring::npos) ? path : path.substr(0, pos);
    if (!sub.empty()) {
      CreateDirectoryW(sub.c_str(), nullptr);
    }
    if (pos == std::wstring::npos) break;
  }

  attrs = GetFileAttributesW(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring GetAppDataDir() {
  PWSTR localAppData = nullptr;
  HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);
  if (FAILED(hr) || !localAppData) return L"";

  std::wstring base(localAppData);
  CoTaskMemFree(localAppData);

  std::wstring dir = JoinPath(base, L"PasteToFile");
  EnsureDirectoryExists(dir);
  return dir;
}

} // namespace ptf
