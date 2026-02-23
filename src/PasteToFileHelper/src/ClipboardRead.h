#pragma once

#include <optional>
#include <string>
#include <vector>
#include <windows.h>

namespace ptf_helper {

struct ClipboardText {
  std::wstring text;
};

struct ClipboardBytes {
  std::vector<uint8_t> bytes;
};

// Reads CF_UNICODETEXT (preferred) or CF_TEXT.
std::optional<ClipboardText> ReadClipboardText();

// Reads the registered formats: "HTML Format" and "Rich Text Format".
std::optional<ClipboardBytes> ReadClipboardHtmlFormat();
std::optional<ClipboardBytes> ReadClipboardRtfFormat();

// Attempts to read an image as an HBITMAP (caller owns and must DeleteObject).
// Prefers CF_BITMAP, then CF_DIBV5/CF_DIB.
std::optional<HBITMAP> ReadClipboardImageAsHbitmap();

} // namespace ptf_helper
