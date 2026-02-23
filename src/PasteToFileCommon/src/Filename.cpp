#include "PasteToFileCommon/Filename.h"

#include "PasteToFileCommon/PathUtils.h"

#include <windows.h>

namespace ptf {

static const wchar_t* Month3Lower(int month1To12) {
  static const wchar_t* kMonths[] = {L"jan", L"feb", L"mar", L"apr", L"may", L"jun",
                                     L"jul", L"aug", L"sep", L"oct", L"nov", L"dec"};
  if (month1To12 < 1 || month1To12 > 12) return L"unk";
  return kMonths[month1To12 - 1];
}

std::wstring BuildDatedBaseName() {
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t buf[64]{};
  swprintf_s(buf, L"PTF-%04u-%s-%02u", st.wYear, Month3Lower(st.wMonth),
             st.wDay);
  return buf;
}

static bool FileExists(const std::wstring& path) {
  DWORD attrs = GetFileAttributesW(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring PickUniquePath(const std::wstring& dir,
                            const std::wstring& extensionWithDot) {
  std::wstring base = BuildDatedBaseName();
  std::wstring p0 = JoinPath(dir, base + extensionWithDot);
  if (!FileExists(p0)) return p0;

  for (int i = 1; i < 1000; i++) {
    wchar_t suffix[16]{};
    swprintf_s(suffix, L"-%02d", i);
    std::wstring p = JoinPath(dir, base + suffix + extensionWithDot);
    if (!FileExists(p)) return p;
  }

  // Fallback: include ticks if pathological.
  ULONGLONG t = GetTickCount64();
  wchar_t buf[32]{};
  swprintf_s(buf, L"-%llu", t);
  return JoinPath(dir, base + buf + extensionWithDot);
}

} // namespace ptf
