#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <shlobj.h>

#include <string>

#include "ShellExtGuids.h"

class ATL_NO_VTABLE CPasteToFileContextMenu
    : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
      public ATL::CComCoClass<CPasteToFileContextMenu, &CLSID_PasteToFileContextMenu>,
      public IShellExtInit,
      public IContextMenu {
public:
  CPasteToFileContextMenu() = default;

  DECLARE_NOT_AGGREGATABLE(CPasteToFileContextMenu)
  DECLARE_NO_REGISTRY()

  BEGIN_COM_MAP(CPasteToFileContextMenu)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IContextMenu)
  END_COM_MAP()

  // IShellExtInit
  STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject* pDataObj, HKEY hkeyProgID) override;

  // IContextMenu
  STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst,
                                UINT idCmdLast, UINT uFlags) override;
  STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override;
  STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved,
                                LPSTR pszName, UINT cchMax) override;

private:
  std::wstring m_targetDir;
  UINT m_idCmdFirst = 0;

  HRESULT ResolveTargetDirFromDataObject(IDataObject* pDataObj);
  HRESULT ResolveTargetDirFromPidl(LPCITEMIDLIST pidlFolder);
};
