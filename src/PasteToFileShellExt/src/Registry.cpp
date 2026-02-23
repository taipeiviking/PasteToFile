#include "Registry.h"

#include <shlwapi.h>

#include <string>

#include "ShellExtGuids.h"

// __ImageBase gives us the HMODULE without storing globals.
extern "C" IMAGE_DOS_HEADER __ImageBase;

static std::wstring GuidToString(const GUID& guid) {
  wchar_t buf[64]{};
  StringFromGUID2(guid, buf, ARRAYSIZE(buf));
  return buf;
}

static std::wstring GetModulePath() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path,
                     ARRAYSIZE(path));
  return path;
}

static LONG SetSzValue(HKEY key, const wchar_t* name, const std::wstring& value) {
  return RegSetValueExW(key, name, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(value.c_str()),
                        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
}

HRESULT RegisterComServer() {
  std::wstring clsid = GuidToString(CLSID_PasteToFileContextMenu);
  std::wstring modulePath = GetModulePath();

  std::wstring keyPath = L"CLSID\\" + clsid;
  HKEY hClsid = nullptr;
  LONG rc = RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, nullptr, 0,
                            KEY_WRITE, nullptr, &hClsid, nullptr);
  if (rc != ERROR_SUCCESS) return HRESULT_FROM_WIN32(rc);

  SetSzValue(hClsid, nullptr, L"PasteToFile Context Menu");

  HKEY hInproc = nullptr;
  rc = RegCreateKeyExW(hClsid, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr,
                       &hInproc, nullptr);
  if (rc != ERROR_SUCCESS) {
    RegCloseKey(hClsid);
    return HRESULT_FROM_WIN32(rc);
  }

  SetSzValue(hInproc, nullptr, modulePath);
  SetSzValue(hInproc, L"ThreadingModel", L"Apartment");

  RegCloseKey(hInproc);
  RegCloseKey(hClsid);
  return S_OK;
}

HRESULT UnregisterComServer() {
  std::wstring clsid = GuidToString(CLSID_PasteToFileContextMenu);
  std::wstring keyPath = L"CLSID\\" + clsid;
  // Best-effort recursive delete.
  LONG rc = SHDeleteKeyW(HKEY_CLASSES_ROOT, keyPath.c_str());
  if (rc == ERROR_FILE_NOT_FOUND) return S_OK;
  return HRESULT_FROM_WIN32(rc);
}

// Exported for regsvr32
extern "C" HRESULT __stdcall DllRegisterServer() { return RegisterComServer(); }
extern "C" HRESULT __stdcall DllUnregisterServer() { return UnregisterComServer(); }
