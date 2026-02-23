#pragma once

#include <windows.h>

namespace ptf {

struct ClipboardFormatsAvailable {
  bool hasText = false;
  bool hasHtml = false;
  bool hasRtf = false;
  bool hasImage = false;
};

UINT GetHtmlClipboardFormat(); // "HTML Format"
UINT GetRtfClipboardFormat();  // "Rich Text Format"

// Uses IsClipboardFormatAvailable (does not require OpenClipboard).
ClipboardFormatsAvailable QueryClipboardFormatsAvailable();

} // namespace ptf
