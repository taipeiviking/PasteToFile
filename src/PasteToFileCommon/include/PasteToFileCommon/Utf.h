#pragma once

#include <string>
#include <string_view>

namespace ptf {

std::string WideToUtf8(std::wstring_view wide);
std::wstring Utf8ToWide(std::string_view utf8);

} // namespace ptf
