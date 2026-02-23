#include "PasteToFileCommon/ClipboardFormats.h"

namespace ptf {

UINT GetHtmlClipboardFormat() {
  static UINT fmt = RegisterClipboardFormatW(L"HTML Format");
  return fmt;
}

UINT GetRtfClipboardFormat() {
  static UINT fmt = RegisterClipboardFormatW(L"Rich Text Format");
  return fmt;
}

ClipboardFormatsAvailable QueryClipboardFormatsAvailable() {
  ClipboardFormatsAvailable out{};

  out.hasText = IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE ||
                IsClipboardFormatAvailable(CF_TEXT) != FALSE;

  UINT html = GetHtmlClipboardFormat();
  if (html != 0) out.hasHtml = IsClipboardFormatAvailable(html) != FALSE;

  UINT rtf = GetRtfClipboardFormat();
  if (rtf != 0) out.hasRtf = IsClipboardFormatAvailable(rtf) != FALSE;

  out.hasImage = IsClipboardFormatAvailable(CF_DIBV5) != FALSE ||
                 IsClipboardFormatAvailable(CF_DIB) != FALSE ||
                 IsClipboardFormatAvailable(CF_BITMAP) != FALSE;

  return out;
}

} // namespace ptf
