#include "ClipboardRead.h"

#include <algorithm>
#include <cstring>

#include "PasteToFileCommon/ClipboardFormats.h"
#include "PasteToFileCommon/Logging.h"

namespace ptf_helper {

static std::optional<std::vector<uint8_t>> ReadClipboardHglobalBytes(UINT format) {
  if (!IsClipboardFormatAvailable(format)) return std::nullopt;
  if (!OpenClipboard(nullptr)) return std::nullopt;

  HANDLE h = GetClipboardData(format);
  if (!h) {
    CloseClipboard();
    return std::nullopt;
  }

  SIZE_T size = GlobalSize(h);
  void* p = GlobalLock(h);
  if (!p || size == 0) {
    if (p) GlobalUnlock(h);
    CloseClipboard();
    return std::nullopt;
  }

  std::vector<uint8_t> bytes;
  bytes.resize(static_cast<size_t>(size));
  memcpy(bytes.data(), p, bytes.size());

  GlobalUnlock(h);
  CloseClipboard();

  // Trim trailing NULs to make file output cleaner.
  while (!bytes.empty() && bytes.back() == 0) bytes.pop_back();
  return bytes;
}

std::optional<ClipboardText> ReadClipboardText() {
  if (!IsClipboardFormatAvailable(CF_UNICODETEXT) &&
      !IsClipboardFormatAvailable(CF_TEXT)) {
    return std::nullopt;
  }

  if (!OpenClipboard(nullptr)) return std::nullopt;

  ClipboardText out{};
  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) {
      CloseClipboard();
      return std::nullopt;
    }
    const wchar_t* p = static_cast<const wchar_t*>(GlobalLock(h));
    if (!p) {
      CloseClipboard();
      return std::nullopt;
    }
    out.text = p;
    GlobalUnlock(h);
    CloseClipboard();
    return out;
  }

  // CF_TEXT fallback (ACP).
  HANDLE h = GetClipboardData(CF_TEXT);
  if (!h) {
    CloseClipboard();
    return std::nullopt;
  }
  const char* p = static_cast<const char*>(GlobalLock(h));
  if (!p) {
    CloseClipboard();
    return std::nullopt;
  }
  std::string s(p);
  GlobalUnlock(h);
  CloseClipboard();

  int needed = MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()),
                                   nullptr, 0);
  if (needed <= 0) return std::nullopt;
  out.text.resize(static_cast<size_t>(needed));
  MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()),
                      out.text.data(), needed);
  return out;
}

std::optional<ClipboardBytes> ReadClipboardHtmlFormat() {
  UINT fmt = ptf::GetHtmlClipboardFormat();
  if (!fmt) return std::nullopt;
  auto bytes = ReadClipboardHglobalBytes(fmt);
  if (!bytes) return std::nullopt;
  return ClipboardBytes{std::move(*bytes)};
}

std::optional<ClipboardBytes> ReadClipboardRtfFormat() {
  UINT fmt = ptf::GetRtfClipboardFormat();
  if (!fmt) return std::nullopt;
  auto bytes = ReadClipboardHglobalBytes(fmt);
  if (!bytes) return std::nullopt;
  return ClipboardBytes{std::move(*bytes)};
}

static std::optional<HBITMAP> CreateHbitmapFromDibGlobal(HGLOBAL hg) {
  SIZE_T size = GlobalSize(hg);
  void* dib = GlobalLock(hg);
  if (!dib || size < sizeof(BITMAPINFOHEADER)) {
    if (dib) GlobalUnlock(hg);
    return std::nullopt;
  }

  const BITMAPINFOHEADER* bih = static_cast<const BITMAPINFOHEADER*>(dib);
  if (bih->biSize < sizeof(BITMAPINFOHEADER)) {
    GlobalUnlock(hg);
    return std::nullopt;
  }

  const BITMAPINFO* bmi = static_cast<const BITMAPINFO*>(dib);

  // Compute palette size (for <= 8bpp).
  DWORD paletteBytes = 0;
  if (bih->biBitCount <= 8) {
    DWORD colors = bih->biClrUsed ? bih->biClrUsed : (1u << bih->biBitCount);
    paletteBytes = colors * sizeof(RGBQUAD);
  } else if (bih->biCompression == BI_BITFIELDS) {
    paletteBytes = 3 * sizeof(DWORD);
  }

  const uint8_t* bits =
      static_cast<const uint8_t*>(dib) + bih->biSize + paletteBytes;
  SIZE_T bitsBytes = size - (bih->biSize + paletteBytes);

  HDC hdc = GetDC(nullptr);
  void* outBits = nullptr;
  HBITMAP hbm = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &outBits, nullptr, 0);
  ReleaseDC(nullptr, hdc);

  if (!hbm || !outBits) {
    GlobalUnlock(hg);
    if (hbm) DeleteObject(hbm);
    return std::nullopt;
  }

  // Avoid overrunning the DIBSection if GlobalSize is larger than the actual pixel data.
  int width = bih->biWidth;
  int height = bih->biHeight < 0 ? -bih->biHeight : bih->biHeight;
  int bpp = bih->biBitCount;
  size_t stride = static_cast<size_t>(((width * bpp + 31) / 32) * 4);
  size_t expected = stride * static_cast<size_t>(height);
  size_t toCopy = (expected < bitsBytes) ? expected : static_cast<size_t>(bitsBytes);
  memcpy(outBits, bits, toCopy);
  GlobalUnlock(hg);
  return hbm;
}

std::optional<HBITMAP> ReadClipboardImageAsHbitmap() {
  if (!IsClipboardFormatAvailable(CF_BITMAP) &&
      !IsClipboardFormatAvailable(CF_DIBV5) &&
      !IsClipboardFormatAvailable(CF_DIB)) {
    return std::nullopt;
  }

  if (!OpenClipboard(nullptr)) return std::nullopt;

  // Prefer CF_BITMAP if available because it is simplest.
  if (IsClipboardFormatAvailable(CF_BITMAP)) {
    HBITMAP src = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
    if (!src) {
      CloseClipboard();
      return std::nullopt;
    }
    // Make our own copy; clipboard owns the original.
    HBITMAP copy = static_cast<HBITMAP>(CopyImage(src, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    CloseClipboard();
    if (!copy) return std::nullopt;
    return copy;
  }

  UINT fmt = IsClipboardFormatAvailable(CF_DIBV5) ? CF_DIBV5 : CF_DIB;
  HGLOBAL hg = static_cast<HGLOBAL>(GetClipboardData(fmt));
  if (!hg) {
    CloseClipboard();
    return std::nullopt;
  }

  auto hbm = CreateHbitmapFromDibGlobal(hg);
  CloseClipboard();
  return hbm;
}

} // namespace ptf_helper
