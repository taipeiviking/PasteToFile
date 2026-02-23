## Manual Test Matrix (v1)

### Setup

- Build `x64` `Release` in Visual Studio.
- Install the shell extension (PowerShell; per-user install does not require admin). Recommended while iterating:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-reinstall-and-restart-explorer.ps1`
- If you want to remove all PasteToFile integration before testing:
  - `powershell -ExecutionPolicy Bypass -File scripts\\reset-all.ps1`
- Restart Explorer if needed:
  - Task Manager -> End task: `Windows Explorer`, then File -> Run new task -> `explorer.exe`

MSI (end-user install):

- `installer\\bin\\x64\\Release\\en-us\\PasteToFileInstaller.msi`
- Installer will stop Explorer during install and restart it on Finish.

Setup EXE (alternate, chains VC++ runtime redistributable; may require admin/UAC):

- `installer\\bin\\x64\\Release\\PasteToFileSetup.exe`

Logs:

- `%LOCALAPPDATA%\\PasteToFile\\ptf.log`
- `ptf-debug.log` (preferred locations):
  - If `PTF_LOG_DIR` is set: `%PTF_LOG_DIR%\\ptf-debug.log`
  - Else: next to the EXE/DLL (if writable)
  - Else: `%LOCALAPPDATA%\\PasteToFile\\ptf-debug.log`

First things to check if “nothing happens”:

1. Confirm `PasteToFileHelper.exe` is installed next to `PasteToFileShellExt.dll`.
2. Open `ptf-debug.log` and look for `[ShellExt] Initialize()` and `[ShellExt] Resolved target dir: ...`
3. If `CreateProcess` fails, the log will show the Win32 error code.

### Tests

Folder background menu:

1. Navigate to any folder in Explorer.
2. Right-click blank area in the folder.
3. Verify `PasteToFile` appears under the classic context menu.

File menu:

1. Right-click any file.
2. Verify `PasteToFile` appears (Windows 11: under `Show more options`).
3. Verify output goes to the file's parent folder.

Clipboard: text only

- Copy some plain text.
- `PasteToFile` should offer:
  - `Paste (auto best)`
  - `Paste as...` -> `Text (.txt)`, `Markdown (.md)`
- Verify output filename format: `PTF-YYYY-mon-DD(.ext)` and collision suffix `-01`, `-02`, ...

Clipboard: image only

- Take a screenshot (or copy an image).
- `PasteToFile` should offer:
  - `Image (PNG)`
- Verify the saved `.png` opens correctly.

Clipboard: HTML + text

- Copy from a browser page (typical clipboard has both HTML and text).
- `PasteToFile` should offer:
  - `HTML (.html)` and text options
  - `Save All Available Formats`
- Verify `.html` contains valid HTML.

Clipboard: RTF

- Copy rich text from Word/WordPad.
- Verify `.rtf` is created and opens in WordPad.

Save all formats

- With clipboard containing multiple formats, choose `Save All Available Formats`.
- Verify multiple files are created (one per available format).

Clipboard history: Win+V (all items)

1. Ensure clipboard history is enabled:
   - Press `Win+V` and enable it if prompted.
2. Copy a few different items (example):
   - Copy plain text.
   - Copy rich text (Word/WordPad).
   - Copy an image (screenshot).
3. Press `Win+V` and confirm you see multiple history items.
4. In Explorer, right-click a folder (Windows 11: use `Show more options`) -> `PasteToFile` ->
   `Save Win+V Clipboard History (All Items)`.
5. Verify multiple `PTF-YYYY-mon-DD-HIST-####.*` files were created.
6. If nothing is created, check `%LOCALAPPDATA%\\PasteToFile\\ptf-debug.log` for history status/errors.

Clear clipboard + history

1. Copy something and press `Win+V` to confirm history has items.
2. Right-click a folder -> `PasteToFile` -> `Clear Clipboard + History`.
3. Verify current clipboard is empty and Win+V history is cleared (pinned items may remain).

### Negative/Edge Cases

- Clipboard locked/busy: `PasteToFile` should not hang Explorer. If actions fail, check log file.
- Very large clipboard contents: confirm no Explorer crash (helper does the heavy lifting).
