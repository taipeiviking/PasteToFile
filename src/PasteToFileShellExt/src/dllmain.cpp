#include <windows.h>

#include <atlbase.h>
#include <atlcom.h>

#include "ShellExtGuids.h"
#include "PasteToFileContextMenu.h"
#include "Module.h"

PasteToFileShellExtModule _AtlModule;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
  return _AtlModule.DllMain(dwReason, lpReserved);
}

// ATL object map
OBJECT_ENTRY_AUTO(CLSID_PasteToFileContextMenu, CPasteToFileContextMenu)
