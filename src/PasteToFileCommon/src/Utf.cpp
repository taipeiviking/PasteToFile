#include "PasteToFileCommon/Utf.h"

#include <windows.h>

namespace ptf {

std::string WideToUtf8(std::wstring_view wide) {
  if (wide.empty()) return {};

  int needed = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                   static_cast<int>(wide.size()), nullptr, 0,
                                   nullptr, nullptr);
  if (needed <= 0) return {};

  std::string out;
  out.resize(static_cast<size_t>(needed));
  int written = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                    static_cast<int>(wide.size()), out.data(),
                                    static_cast<int>(out.size()), nullptr,
                                    nullptr);
  if (written != needed) return {};
  return out;
}

std::wstring Utf8ToWide(std::string_view utf8) {
  if (utf8.empty()) return {};

  int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                   static_cast<int>(utf8.size()), nullptr, 0);
  if (needed <= 0) return {};

  std::wstring out;
  out.resize(static_cast<size_t>(needed));
  int written =
      MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                          static_cast<int>(utf8.size()), out.data(), needed);
  if (written != needed) return {};
  return out;
}

} // namespace ptf
