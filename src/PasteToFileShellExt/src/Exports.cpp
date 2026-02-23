#include <windows.h>

#include "Module.h"

extern "C" HRESULT __stdcall DllCanUnloadNow() {
  return _AtlModule.DllCanUnloadNow();
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid,
                                               LPVOID* ppv) {
  return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}
