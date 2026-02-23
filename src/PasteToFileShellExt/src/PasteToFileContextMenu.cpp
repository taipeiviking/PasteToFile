#include "PasteToFileContextMenu.h"

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <string>

#include "PasteToFileCommon/ClipboardFormats.h"
#include "PasteToFileCommon/Logging.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {

constexpr UINT kCmdAuto = 0;
constexpr UINT kCmdTextTxt = 1;
constexpr UINT kCmdTextMd = 2;
constexpr UINT kCmdHtml = 3;
constexpr UINT kCmdRtf = 4;
constexpr UINT kCmdPng = 5;
constexpr UINT kCmdAll = 6;
constexpr UINT kCmdHistoryAll = 7;
constexpr UINT kCmdClearAll = 8;
constexpr UINT kCmdCount = 9;

static void InsertItem(HMENU menu, const wchar_t* text, UINT id, bool enabled = true) {
  MENUITEMINFOW mii{};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
  mii.fType = MFT_STRING;
  mii.fState = enabled ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED);
  mii.wID = id;
  mii.dwTypeData = const_cast<wchar_t*>(text);
  UINT pos = GetMenuItemCount(menu);
  InsertMenuItemW(menu, pos, TRUE, &mii);
}

static void InsertSeparator(HMENU menu) {
  MENUITEMINFOW mii{};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_SEPARATOR;
  UINT pos = GetMenuItemCount(menu);
  InsertMenuItemW(menu, pos, TRUE, &mii);
}

static void InsertPopup(HMENU parent, const wchar_t* text, HMENU popup) {
  MENUITEMINFOW mii{};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
  mii.fType = MFT_STRING;
  mii.hSubMenu = popup;
  mii.dwTypeData = const_cast<wchar_t*>(text);
  UINT pos = GetMenuItemCount(parent);
  InsertMenuItemW(parent, pos, TRUE, &mii);
}

static std::wstring GetModuleDir() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path, ARRAYSIZE(path));
  PathRemoveFileSpecW(path);
  return path;
}

static bool LaunchHelper(const std::wstring& targetDir, const wchar_t* action) {
  std::wstring helperPath = GetModuleDir() + L"\\PasteToFileHelper.exe";

  std::wstring cmd =
      L"\"" + helperPath + L"\" --target \"" + targetDir + L"\" --action " + action;

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  DWORD flags = CREATE_NO_WINDOW | DETACHED_PROCESS;

  // CreateProcess requires mutable command line buffer.
  std::wstring mutableCmd = cmd;
  BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, flags,
                           nullptr, nullptr, &si, &pi);
  if (!ok) {
    ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                      L"[ShellExt] CreateProcess failed: " + helperPath + L" err=" +
                          std::to_wstring(GetLastError()));
    return false;
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

} // namespace

STDMETHODIMP CPasteToFileContextMenu::Initialize(LPCITEMIDLIST pidlFolder,
                                                IDataObject* pDataObj,
                                                HKEY /*hkeyProgID*/) {
  m_targetDir.clear();
  ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                    L"[ShellExt] Initialize()");

  HRESULT hr = E_FAIL;
  if (pDataObj) {
    hr = ResolveTargetDirFromDataObject(pDataObj);
    if (SUCCEEDED(hr) && !m_targetDir.empty()) return S_OK;
  }

  if (pidlFolder) {
    hr = ResolveTargetDirFromPidl(pidlFolder);
    if (SUCCEEDED(hr) && !m_targetDir.empty()) return S_OK;
  }

  ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                    L"[ShellExt] Initialize failed to resolve target dir");
  return E_FAIL;
}

HRESULT CPasteToFileContextMenu::ResolveTargetDirFromPidl(LPCITEMIDLIST pidlFolder) {
  wchar_t path[MAX_PATH]{};
  if (!SHGetPathFromIDListW(pidlFolder, path)) return E_FAIL;
  m_targetDir = path;
  return S_OK;
}

HRESULT CPasteToFileContextMenu::ResolveTargetDirFromDataObject(IDataObject* pDataObj) {
  FORMATETC fe{};
  fe.cfFormat = CF_HDROP;
  fe.ptd = nullptr;
  fe.dwAspect = DVASPECT_CONTENT;
  fe.lindex = -1;
  fe.tymed = TYMED_HGLOBAL;

  STGMEDIUM stm{};
  HRESULT hr = pDataObj->GetData(&fe, &stm);
  if (FAILED(hr)) return hr;

  HDROP hdrop = static_cast<HDROP>(stm.hGlobal);
  UINT count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
  if (count < 1) {
    ReleaseStgMedium(&stm);
    return E_FAIL;
  }

  wchar_t path[MAX_PATH]{};
  if (DragQueryFileW(hdrop, 0, path, ARRAYSIZE(path)) == 0) {
    ReleaseStgMedium(&stm);
    return E_FAIL;
  }

  ReleaseStgMedium(&stm);

  DWORD attrs = GetFileAttributesW(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) return E_FAIL;
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    m_targetDir = path;
  } else {
    // If the user right-clicks a file, use its parent directory as output.
    wchar_t dirPath[MAX_PATH]{};
    wcscpy_s(dirPath, path);
    PathRemoveFileSpecW(dirPath);
    m_targetDir = dirPath;
  }

  ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                    L"[ShellExt] Resolved target dir: " + m_targetDir);
  return S_OK;
}

STDMETHODIMP CPasteToFileContextMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu,
                                                      UINT idCmdFirst,
                                                      UINT /*idCmdLast*/,
                                                      UINT uFlags) {
  if (uFlags & CMF_DEFAULTONLY) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

  m_idCmdFirst = idCmdFirst;

  HMENU rootPopup = CreatePopupMenu();
  if (!rootPopup) return E_FAIL;

  auto avail = ptf::QueryClipboardFormatsAvailable();
  ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                    L"[ShellExt] QueryContextMenu target=" + m_targetDir +
                        L" text=" + std::to_wstring(avail.hasText) +
                        L" html=" + std::to_wstring(avail.hasHtml) +
                        L" rtf=" + std::to_wstring(avail.hasRtf) +
                        L" img=" + std::to_wstring(avail.hasImage));
  int saveableCount = (avail.hasText ? 1 : 0) + (avail.hasHtml ? 1 : 0) +
                      (avail.hasRtf ? 1 : 0) + (avail.hasImage ? 1 : 0);

  if (saveableCount == 0) {
    InsertItem(rootPopup, L"Clipboard has no supported data", idCmdFirst + kCmdAuto,
               false);
  } else {
    InsertItem(rootPopup, L"Paste (auto best)", idCmdFirst + kCmdAuto, true);
    InsertSeparator(rootPopup);

    HMENU asPopup = CreatePopupMenu();
    if (avail.hasText) {
      InsertItem(asPopup, L"Text (.txt)", idCmdFirst + kCmdTextTxt, true);
      InsertItem(asPopup, L"Markdown (.md)", idCmdFirst + kCmdTextMd, true);
    }
    if (avail.hasHtml) InsertItem(asPopup, L"HTML (.html)", idCmdFirst + kCmdHtml, true);
    if (avail.hasRtf) InsertItem(asPopup, L"RTF (.rtf)", idCmdFirst + kCmdRtf, true);
    if (avail.hasImage) InsertItem(asPopup, L"Image (PNG)", idCmdFirst + kCmdPng, true);

    InsertPopup(rootPopup, L"Paste as...", asPopup);

    if (saveableCount >= 2) {
      InsertSeparator(rootPopup);
      InsertItem(rootPopup, L"Save All Available Formats", idCmdFirst + kCmdAll, true);
    }

    // Clipboard history export (Win+V). This uses WinRT in the helper and may fail if
    // clipboard history is disabled by the user or policy.
    InsertSeparator(rootPopup);
    InsertItem(rootPopup, L"Save Win+V Clipboard History (All Items)",
               idCmdFirst + kCmdHistoryAll, true);
  }

  // Utility actions (always available).
  InsertSeparator(rootPopup);
  InsertItem(rootPopup, L"Clear Clipboard + History", idCmdFirst + kCmdClearAll, true);

  MENUITEMINFOW mii{};
  mii.cbSize = sizeof(mii);
  mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<wchar_t*>(L"PasteToFile");
  mii.hSubMenu = rootPopup;

  if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii)) return E_FAIL;

  // Reserve our full command-id range even if some items were omitted.
  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, kCmdCount);
}

STDMETHODIMP CPasteToFileContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
  if (!pici) return E_INVALIDARG;
  if (HIWORD(pici->lpVerb) != 0) return E_FAIL; // not handling canonical verbs

  // Explorer passes the verb as an offset (id - idCmdFirst), not the absolute menu ID.
  UINT verb = LOWORD(pici->lpVerb);

  UINT offset = verb;
  if (offset >= kCmdCount) {
    // Be defensive in case a host passes absolute IDs.
    if (verb >= m_idCmdFirst && verb < (m_idCmdFirst + kCmdCount)) {
      offset = verb - m_idCmdFirst;
    } else {
      ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                        L"[ShellExt] InvokeCommand invalid verb=" + std::to_wstring(verb) +
                            L" idCmdFirst=" + std::to_wstring(m_idCmdFirst));
      return E_INVALIDARG;
    }
  }
  if (m_targetDir.empty()) return E_FAIL;

  const wchar_t* action = L"auto";
  switch (offset) {
    case kCmdAuto: action = L"auto"; break;
    case kCmdTextTxt: action = L"text-txt"; break;
    case kCmdTextMd: action = L"text-md"; break;
    case kCmdHtml: action = L"html"; break;
    case kCmdRtf: action = L"rtf"; break;
    case kCmdPng: action = L"png"; break;
    case kCmdAll: action = L"all"; break;
    case kCmdHistoryAll: action = L"history-all"; break;
    case kCmdClearAll: action = L"clear-all"; break;
  }

  ptf::LogLineDebug(reinterpret_cast<HMODULE>(&__ImageBase), L"ptf-debug.log",
                    L"[ShellExt] InvokeCommand action=" + std::wstring(action) +
                        L" target=" + m_targetDir);

  if (!LaunchHelper(m_targetDir, action)) return E_FAIL;
  return S_OK;
}

STDMETHODIMP CPasteToFileContextMenu::GetCommandString(UINT_PTR /*idCmd*/,
                                                       UINT /*uType*/,
                                                       UINT* /*pReserved*/,
                                                       LPSTR /*pszName*/,
                                                       UINT /*cchMax*/) {
  return E_NOTIMPL;
}
