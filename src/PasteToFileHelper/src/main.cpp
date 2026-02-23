#include <windows.h>

#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

#include "ClipboardRead.h"
#include "ImageWritePng.h"
#include "TextWrite.h"

#include "PasteToFileCommon/ClipboardFormats.h"
#include "PasteToFileCommon/Filename.h"
#include "PasteToFileCommon/Logging.h"

namespace {

enum class Action {
  AutoBest,
  TextTxt,
  TextMd,
  Html,
  Rtf,
  ImagePng,
  SaveAll,
  HistoryAll,
  ClearAll,
};

static bool ArgEquals(const wchar_t* a, const wchar_t* b) {
  return _wcsicmp(a, b) == 0;
}

static std::wstring GetArgValue(int argc, wchar_t** argv, const wchar_t* name) {
  for (int i = 1; i + 1 < argc; i++) {
    if (ArgEquals(argv[i], name)) return argv[i + 1];
  }
  return L"";
}

static Action ParseAction(const std::wstring& s) {
  if (_wcsicmp(s.c_str(), L"auto") == 0) return Action::AutoBest;
  if (_wcsicmp(s.c_str(), L"text-txt") == 0) return Action::TextTxt;
  if (_wcsicmp(s.c_str(), L"text-md") == 0) return Action::TextMd;
  if (_wcsicmp(s.c_str(), L"html") == 0) return Action::Html;
  if (_wcsicmp(s.c_str(), L"rtf") == 0) return Action::Rtf;
  if (_wcsicmp(s.c_str(), L"png") == 0) return Action::ImagePng;
  if (_wcsicmp(s.c_str(), L"all") == 0) return Action::SaveAll;
  if (_wcsicmp(s.c_str(), L"history-all") == 0) return Action::HistoryAll;
  if (_wcsicmp(s.c_str(), L"clear-all") == 0) return Action::ClearAll;
  return Action::AutoBest;
}

// HTML clipboard format includes a header; try to extract the actual HTML section.
static std::vector<uint8_t> ExtractHtmlPayloadOrOriginal(const std::vector<uint8_t>& bytes) {
  auto asString = std::string(bytes.begin(), bytes.end());
  auto findNum = [&](const char* key) -> int {
    size_t pos = asString.find(key);
    if (pos == std::string::npos) return -1;
    pos += strlen(key);
    while (pos < asString.size() && (asString[pos] == ' ')) pos++;
    int val = 0;
    bool any = false;
    while (pos < asString.size() && asString[pos] >= '0' && asString[pos] <= '9') {
      any = true;
      val = (val * 10) + (asString[pos] - '0');
      pos++;
    }
    return any ? val : -1;
  };

  int startHtml = findNum("StartHTML:");
  int endHtml = findNum("EndHTML:");
  if (startHtml >= 0 && endHtml > startHtml &&
      endHtml <= static_cast<int>(bytes.size())) {
    return std::vector<uint8_t>(bytes.begin() + startHtml, bytes.begin() + endHtml);
  }

  return bytes;
}

static bool SaveText(const std::wstring& dir, const std::wstring& ext, const std::wstring& text) {
  std::wstring outPath;
  bool ok = ptf_helper::WriteUtf8TextFileUnique(dir, ext, text, &outPath);
  if (ok) ptf::LogLine(L"Saved text: " + outPath);
  return ok;
}

static bool SaveBytes(const std::wstring& dir, const std::wstring& ext, const std::vector<uint8_t>& bytes) {
  std::wstring outPath;
  bool ok = ptf_helper::WriteBinaryFileUnique(dir, ext, bytes, &outPath);
  if (ok) ptf::LogLine(L"Saved bytes: " + outPath);
  return ok;
}

static bool SaveImagePng(const std::wstring& dir, HBITMAP hbm) {
  std::wstring outPath;
  bool ok = ptf_helper::WritePngFileUniqueFromHbitmap(dir, hbm, &outPath);
  if (ok) ptf::LogLine(L"Saved png: " + outPath);
  return ok;
}

static std::wstring HistoryBaseName(int index1Based) {
  wchar_t buf[96]{};
  swprintf_s(buf, L"%s-HIST-%04d", ptf::BuildDatedBaseName().c_str(), index1Based);
  return buf;
}

static std::vector<uint8_t> ReadAllBytesFromRandomAccessStream(
    const winrt::Windows::Storage::Streams::IRandomAccessStream& stream) {
  using namespace winrt::Windows::Storage::Streams;

  uint64_t size64 = stream.Size();
  if (size64 == 0) return {};
  if (size64 > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) return {};
  uint32_t size = static_cast<uint32_t>(size64);

  auto input = stream.GetInputStreamAt(0);
  Buffer buf(size);
  auto readBuf = input.ReadAsync(buf, size, InputStreamOptions::None).get();
  auto reader = DataReader::FromBuffer(readBuf);

  std::vector<uint8_t> bytes(reader.UnconsumedBufferLength());
  if (!bytes.empty()) {
    reader.ReadBytes(winrt::array_view<uint8_t>(bytes));
  }
  return bytes;
}

static bool SaveClipboardHistoryAll(const std::wstring& targetDir) {
  using namespace winrt::Windows::ApplicationModel::DataTransfer;

  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] history-all: start");
  try {
    ClipboardHistoryItemsResult result = Clipboard::GetHistoryItemsAsync().get();
    auto status = result.Status();
    if (status != ClipboardHistoryItemsResultStatus::Success) {
      ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                        L"[Helper] history-all: status=" +
                            std::to_wstring(static_cast<int>(status)));
      return false;
    }

    auto items = result.Items();
    uint32_t count = items.Size();
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] history-all: items=" + std::to_wstring(count));
    if (count == 0) return false;

    bool any = false;
    bool allOk = true;

    for (uint32_t i = 0; i < count; i++) {
      int index1 = static_cast<int>(i) + 1;
      auto item = items.GetAt(i);
      auto content = item.Content();
      std::wstring baseName = HistoryBaseName(index1);

      if (content.Contains(StandardDataFormats::Text())) {
        any = true;
        auto text = content.GetTextAsync().get();
        allOk = ptf_helper::WriteUtf8TextFileUniqueWithBase(
                    targetDir, baseName, L".txt", text.c_str(), nullptr) &&
                allOk;
      }
      if (content.Contains(StandardDataFormats::Html())) {
        any = true;
        auto html = content.GetHtmlFormatAsync().get();
        allOk = ptf_helper::WriteUtf8TextFileUniqueWithBase(
                    targetDir, baseName, L".html", html.c_str(), nullptr) &&
                allOk;
      }
      if (content.Contains(StandardDataFormats::Rtf())) {
        any = true;
        auto rtf = content.GetRtfAsync().get();
        allOk = ptf_helper::WriteUtf8TextFileUniqueWithBase(
                    targetDir, baseName, L".rtf", rtf.c_str(), nullptr) &&
                allOk;
      }
      if (content.Contains(StandardDataFormats::Bitmap())) {
        any = true;
        auto bmpRef = content.GetBitmapAsync().get();
        auto stream = bmpRef.OpenReadAsync().get();
        auto bytes = ReadAllBytesFromRandomAccessStream(stream);
        if (bytes.empty()) {
          ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                            L"[Helper] history-all: bitmap empty");
          allOk = false && allOk;
        } else {
          allOk = ptf_helper::WritePngFileUniqueFromEncodedImageBytesWithBase(
                      targetDir, baseName, bytes, nullptr) &&
                  allOk;
        }
      }
    }

    return any && allOk;
  } catch (const winrt::hresult_error& e) {
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] history-all: exception hr=" +
                          std::to_wstring(static_cast<int32_t>(e.code())) +
                          L" msg=" + std::wstring(e.message().c_str()));
    return false;
  }
}

static bool ClearClipboardAndHistory() {
  using namespace winrt::Windows::ApplicationModel::DataTransfer;

  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] clear-all: start");

  bool clearedClipboard = false;
  if (OpenClipboard(nullptr)) {
    clearedClipboard = EmptyClipboard() != FALSE;
    CloseClipboard();
  } else {
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] clear-all: OpenClipboard failed err=" +
                          std::to_wstring(GetLastError()));
  }

  bool clearedHistory = false;
  try {
    clearedHistory = Clipboard::ClearHistory();
  } catch (const winrt::hresult_error& e) {
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] clear-all: ClearHistory exception hr=" +
                          std::to_wstring(static_cast<int32_t>(e.code())) +
                          L" msg=" + std::wstring(e.message().c_str()));
  }

  ptf::LogLineDebug(
      GetModuleHandleW(nullptr), L"ptf-debug.log",
      L"[Helper] clear-all: clearedClipboard=" + std::to_wstring(clearedClipboard) +
          L" clearedHistory=" + std::to_wstring(clearedHistory) +
          L" (note: pinned Win+V items may remain)");

  return clearedClipboard || clearedHistory;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
  winrt::init_apartment(winrt::apartment_type::single_threaded);
  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] start argc=" + std::to_wstring(argc));

  Action action = ParseAction(GetArgValue(argc, argv, L"--action"));
  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] action=" +
                        std::to_wstring(static_cast<int>(action)) +
                        L" (parsed)");

  std::wstring targetDir = GetArgValue(argc, argv, L"--target");
  if (targetDir.empty() && action != Action::ClearAll) {
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] missing --target");
    winrt::uninit_apartment();
    return 2;
  }

  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] target=" + (targetDir.empty() ? L"(none)" : targetDir));

  auto avail = ptf::QueryClipboardFormatsAvailable();
  ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                    L"[Helper] clipboard avail text=" + std::to_wstring(avail.hasText) +
                        L" html=" + std::to_wstring(avail.hasHtml) +
                        L" rtf=" + std::to_wstring(avail.hasRtf) +
                        L" img=" + std::to_wstring(avail.hasImage));

  auto doAuto = [&]() -> bool {
    if (avail.hasImage) {
      auto hbm = ptf_helper::ReadClipboardImageAsHbitmap();
      if (!hbm) return false;
      bool ok = SaveImagePng(targetDir, *hbm);
      DeleteObject(*hbm);
      return ok;
    }
    if (avail.hasHtml) {
      auto html = ptf_helper::ReadClipboardHtmlFormat();
      if (!html) return false;
      auto payload = ExtractHtmlPayloadOrOriginal(html->bytes);
      return SaveBytes(targetDir, L".html", payload);
    }
    if (avail.hasRtf) {
      auto rtf = ptf_helper::ReadClipboardRtfFormat();
      if (!rtf) return false;
      return SaveBytes(targetDir, L".rtf", rtf->bytes);
    }
    if (avail.hasText) {
      auto text = ptf_helper::ReadClipboardText();
      if (!text) return false;
      return SaveText(targetDir, L".txt", text->text);
    }
    return false;
  };

  bool ok = false;
  switch (action) {
    case Action::AutoBest:
      ok = doAuto();
      break;
    case Action::TextTxt: {
      auto text = ptf_helper::ReadClipboardText();
      ok = text && SaveText(targetDir, L".txt", text->text);
      break;
    }
    case Action::TextMd: {
      auto text = ptf_helper::ReadClipboardText();
      ok = text && SaveText(targetDir, L".md", text->text);
      break;
    }
    case Action::Html: {
      auto html = ptf_helper::ReadClipboardHtmlFormat();
      ok = html && SaveBytes(targetDir, L".html", ExtractHtmlPayloadOrOriginal(html->bytes));
      break;
    }
    case Action::Rtf: {
      auto rtf = ptf_helper::ReadClipboardRtfFormat();
      ok = rtf && SaveBytes(targetDir, L".rtf", rtf->bytes);
      break;
    }
    case Action::ImagePng: {
      auto hbm = ptf_helper::ReadClipboardImageAsHbitmap();
      ok = hbm && SaveImagePng(targetDir, *hbm);
      if (hbm) DeleteObject(*hbm);
      break;
    }
    case Action::SaveAll: {
      bool any = false;
      bool allOk = true;

      if (avail.hasText) {
        auto text = ptf_helper::ReadClipboardText();
        if (text) {
          any = true;
          allOk = SaveText(targetDir, L".txt", text->text) && allOk;
        }
      }
      if (avail.hasHtml) {
        auto html = ptf_helper::ReadClipboardHtmlFormat();
        if (html) {
          any = true;
          allOk = SaveBytes(targetDir, L".html", ExtractHtmlPayloadOrOriginal(html->bytes)) && allOk;
        }
      }
      if (avail.hasRtf) {
        auto rtf = ptf_helper::ReadClipboardRtfFormat();
        if (rtf) {
          any = true;
          allOk = SaveBytes(targetDir, L".rtf", rtf->bytes) && allOk;
        }
      }
      if (avail.hasImage) {
        auto hbm = ptf_helper::ReadClipboardImageAsHbitmap();
        if (hbm) {
          any = true;
          allOk = SaveImagePng(targetDir, *hbm) && allOk;
          DeleteObject(*hbm);
        }
      }

      ok = any && allOk;
      break;
    }
    case Action::HistoryAll:
      ok = SaveClipboardHistoryAll(targetDir);
      break;
    case Action::ClearAll:
      ok = ClearClipboardAndHistory();
      break;
  }

  if (!ok) {
    ptf::LogLineDebug(GetModuleHandleW(nullptr), L"ptf-debug.log",
                      L"[Helper] failed");
  }
  winrt::uninit_apartment();
  return ok ? 0 : 1;
}
