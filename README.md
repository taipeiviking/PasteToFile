# PasteToFile

PasteToFile is a Windows Explorer shell extension that adds a classic context-menu submenu
named `PasteToFile`. It saves the current clipboard contents (text, HTML, RTF, images) into
new file(s) in the folder you right-click.

It also supports exporting **Win+V clipboard history** and clearing the clipboard + history.

## Quick demo / how-to video

After installing, the Finish page includes a checkbox to open this video:

- https://youtu.be/HouPdC6vzxA

## Supported Windows versions

- Windows 10: classic right-click menu
- Windows 11: `Show more options` (classic menu)

This project intentionally targets the **classic** menu handler (`IContextMenu`). It does not
integrate into the Windows 11 “first-level” context menu.

## Features

- **Paste (auto best)**: saves the best available clipboard format into a file
- **Paste as...**
  - `Text (.txt)`
  - `Markdown (.md)`
  - `HTML (.html)`
  - `RTF (.rtf)`
  - `Image (PNG)`
- **Save All Available Formats**: when multiple formats exist, saves one file per format
- **Save Win+V Clipboard History (All Items)**: exports Windows clipboard history items
- **Clear Clipboard + History**: clears current clipboard and requests Win+V history clear (pinned items may remain)

## Output filenames

PasteToFile writes files like:

- `PTF-YYYY-mon-DD.txt`
- `PTF-YYYY-mon-DD-01.png` (collision suffixes: `-01`, `-02`, ...)
- `PTF-YYYY-mon-DD-HIST-0001.html` (history export)

## Install (recommended)

Build and run the MSI (it includes app-local VC++ runtime DLLs to reduce dependency issues):

- `installer/bin/x64/Release/en-us/PasteToFileInstaller.msi`

Optional setup EXE (includes the VC++ runtime redistributable as a chained prerequisite; may trigger UAC):

- `installer/bin/x64/Release/PasteToFileSetup.exe`

Notes:

- Default install scope is **per-user** (no admin prompt).
- Explorer loads shell extensions in-process; the installer will stop Explorer during install
  and restart it at the end so the menu can appear.
- Finish page can optionally open the how-to video in your default browser.

If you install from a terminal, use:

- `msiexec /i "C:\\path\\to\\PasteToFileInstaller.msi"`

## How to use

1. Copy something to the clipboard (text, image, etc.)
2. In Explorer, right-click:
   - folder background, folder item, or file
3. Windows 11: click `Show more options`
4. Choose `PasteToFile` and pick an action

## Logs

PasteToFile writes two logs:

- `ptf.log`: `%LOCALAPPDATA%\\PasteToFile\\ptf.log`
- `ptf-debug.log` (more verbose):
  - If `PTF_LOG_DIR` is set: `%PTF_LOG_DIR%\\ptf-debug.log`
  - Else: next to the EXE/DLL (if writable)
  - Else: `%LOCALAPPDATA%\\PasteToFile\\ptf-debug.log`

## Build (developers)

Open and build:

- `PasteToFile.sln` (x64, Debug or Release)

Projects:

- `src/PasteToFileShellExt`: in-proc COM DLL (classic Explorer context menu)
- `src/PasteToFileHelper`: out-of-proc helper EXE (clipboard read/convert/write, WinRT clipboard history)
- `src/PasteToFileCommon`: shared utilities (logging, filenames, UTF helpers, clipboard format detection)

### Dev install / iterate fast

Shell extensions run inside `explorer.exe`, so the DLL you built is not necessarily the one Explorer is using.
Use the dev scripts while iterating:

- Install to `%LOCALAPPDATA%\\PasteToFile`:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-install-localappdata.ps1`
- Uninstall from `%LOCALAPPDATA%\\PasteToFile`:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-uninstall-localappdata.ps1`
- Full reinstall + restart Explorer:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-reinstall-and-restart-explorer.ps1`

### Automated helper tests (no Explorer required)

Runs a PowerShell test harness that sets clipboard contents and verifies created files (without using the shell extension UI):

- `powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File scripts\\test-helper.ps1 -Configuration Release -Platform x64 -KeepOutput`

## Uninstall / reset everything (developers)

To remove registrations (and optionally installed files/logs):

- `powershell -ExecutionPolicy Bypass -File scripts\\reset-all.ps1`
- `powershell -ExecutionPolicy Bypass -File scripts\\reset-all.ps1 -DeleteFiles`

## Troubleshooting

- **Menu not showing (Windows 11)**: use `Show more options`, then look for `PasteToFile`.
- **Clicking menu does nothing**: check `ptf-debug.log` and confirm `PasteToFileHelper.exe` exists next to the shell extension DLL.
- **Files not created**: confirm you’re right-clicking a folder background/item (or a file; output goes to the file’s parent folder).
- **History export fails**: `Win+V` clipboard history must be enabled and allowed by policy.

More:

- `docs/MANUAL_TESTS.md`
- `docs/DEVELOPMENT.md`
- `docs/ARCHITECTURE.md`
- `docs/INSTALLER.md`

## License

Not specified yet. Add a `LICENSE` file if you plan to publish publicly.
