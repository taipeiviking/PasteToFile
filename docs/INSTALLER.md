# Installer (MSI) Notes

The end-user installer is built with WiX Toolset and is located under `installer/`.

Output MSI:

- `installer/bin/x64/Release/en-us/PasteToFileInstaller.msi`

Optional setup EXE (chains the official VC++ redistributable; may trigger UAC/admin):

- `installer/bin/x64/Release/PasteToFileSetup.exe`

## Dependency strategy

To reduce install-time failures on machines missing the VC++ runtime, the MSI includes the required
MSVC runtime DLLs app-local (next to `PasteToFileShellExt.dll` and `PasteToFileHelper.exe`).

The setup EXE exists as an alternate path that can install the official VC++ redistributable.

## Build

From a Developer PowerShell (or any shell where MSBuild is available):

1) Build the solution (produces `bin/x64/Release/*.dll` and `bin/x64/Release/*.exe`):

- `MSBuild PasteToFile.sln /m /t:Build /p:Configuration=Release /p:Platform=x64`

2) Build the MSI (restores WiX NuGet packages):

- `MSBuild installer/PasteToFileInstaller.wixproj /restore /t:Build /p:Configuration=Release /p:Platform=x64`

3) Build the setup bundle EXE (downloads `vc_redist.x64.exe` at build time, then bundles it + the MSI):

- `MSBuild installer/PasteToFileSetup.wixproj /restore /t:Build /p:Configuration=Release /p:Platform=x64`

Notes:

- The MSI build runs `scripts/stage-vc-runtime.ps1` to stage the runtime DLLs into `bin/x64/Release/` before linking.
- The staging script uses the VS redist folder when available, otherwise falls back to the system runtime in `C:\Windows\Sysnative`.

## Goals

- Per-user install by default (fast, no admin prompt)
- Clear, step-by-step UI with screenshots
- Avoid “files in use” problems from Explorer holding the in-proc DLL
- Optional “watch how-to video” on Finish

## UI assets

- Dialog background (Welcome/Finish): `installer/assets/ptf_logo_493x312.png`
  - Set via `WixUIDialogBmp`
- Banner image (top strip): `installer/assets/ptf_logo_493x58.png`
  - Set via `WixUIBannerBmp`
- How-to page content (RTF):
  - Template: `installer/assets/howto-template.rtf`
  - Generated: `installer/assets/howto.rtf`
  - Generator: `installer/scripts/gen-howto-rtf.ps1`
  - Screenshots: `images/Menu1.png`, `images/Menu2.png`

## Product icon

The icon used by Apps & Features / Programs and Features is:

- `installer/assets/PTF_app.ico`

It is referenced from `installer/Product.wxs` via `ARPPRODUCTICON`.

## Explorer stop/restart behavior

Because the shell extension DLL is loaded in-process by `explorer.exe`, the MSI may otherwise
hit file-lock issues during upgrade/repair.

The MSI:

- disables Restart Manager app-shutdown attempts (`MSIRESTARTMANAGERCONTROL=Disable`)
- stops Explorer during install (execute sequence, before `InstallFiles`)
- restarts Explorer when the user clicks Finish

Important: restarting Explorer closes open Explorer windows.

## Finish checkbox: open video

The Finish dialog includes an optional checkbox:

- “Watch the how-to video (opens your browser)”

When checked, clicking Finish opens:

- https://youtu.be/HouPdC6vzxA

