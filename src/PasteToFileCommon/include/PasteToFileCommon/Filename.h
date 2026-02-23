#pragma once

#include <string>

namespace ptf {

// Builds a base name like: PTF-2026-feb-23
std::wstring BuildDatedBaseName();

// Returns a full path with a collision-safe suffix:
//   <dir>\\PTF-YYYY-mon-DD.ext
//   <dir>\\PTF-YYYY-mon-DD-01.ext
//   ...
std::wstring PickUniquePath(const std::wstring& dir, const std::wstring& extensionWithDot);

} // namespace ptf
