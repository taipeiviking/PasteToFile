#include "PasteToFileCommon/Logging.h"

#include "PasteToFileCommon/PathUtils.h"

#include <windows.h>

namespace ptf {

static std::wstring NowStamp() {
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t buf[64]{};
  swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u:%02u.%03u", st.wYear, st.wMonth,
             st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
  return buf;
}

static bool AppendLineToFile(const std::wstring& path, const std::wstring& line) {
  HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;

  std::wstring out = NowStamp() + L" " + line + L"\r\n";
  DWORD bytes = 0;
  BOOL ok = WriteFile(h, out.data(), static_cast<DWORD>(out.size() * sizeof(wchar_t)),
                      &bytes, nullptr);
  CloseHandle(h);
  return ok != FALSE;
}

void LogLine(const std::wstring& line) {
  std::wstring dir = GetAppDataDir();
  if (dir.empty()) return;

  std::wstring path = JoinPath(dir, L"ptf.log");
  AppendLineToFile(path, line);
}

static std::wstring GetModuleDir(HMODULE moduleForDir) {
  wchar_t path[MAX_PATH]{};
  DWORD n = GetModuleFileNameW(moduleForDir, path, ARRAYSIZE(path));
  if (n == 0 || n >= ARRAYSIZE(path)) return L"";
  std::wstring s(path);
  size_t pos = s.find_last_of(L"\\/");
  if (pos == std::wstring::npos) return L"";
  return s.substr(0, pos);
}

static std::wstring GetEnvVar(const wchar_t* name) {
  wchar_t buf[4096]{};
  DWORD n = GetEnvironmentVariableW(name, buf, ARRAYSIZE(buf));
  if (n == 0 || n >= ARRAYSIZE(buf)) return L"";
  return buf;
}

void LogLineDebug(HMODULE moduleForDir, const std::wstring& fileName,
                  const std::wstring& line) {
  // 1) Explicit log dir via env var (best for debugging).
  std::wstring envDir = GetEnvVar(L"PTF_LOG_DIR");
  if (!envDir.empty()) {
    std::wstring p = JoinPath(envDir, fileName);
    if (AppendLineToFile(p, line)) return;
  }

  // 2) Try next to the module (EXE/DLL) if the directory is writable.
  std::wstring moduleDir = GetModuleDir(moduleForDir);
  if (!moduleDir.empty()) {
    std::wstring p = JoinPath(moduleDir, fileName);
    if (AppendLineToFile(p, line)) return;
  }

  // 3) Fallback: LocalAppData.
  std::wstring dir = GetAppDataDir();
  if (dir.empty()) return;
  AppendLineToFile(JoinPath(dir, fileName), line);
}

} // namespace ptf
