# Architecture

PasteToFile is implemented as a classic Explorer context-menu shell extension (in-proc) that
delegates all heavy work to a separate helper process (out-of-proc).

## Components

### `PasteToFileShellExt.dll` (in-proc)

- Location: `src/PasteToFileShellExt`
- Technology: COM in-proc DLL implementing the classic context menu handler (`IContextMenu`)
- Responsibilities:
  - Determine the target output directory (folder background, folder item, or file -> parent folder)
  - Build a dynamic `PasteToFile` submenu based on clipboard formats
  - Launch `PasteToFileHelper.exe` with `--target "<dir>" --action <action>`
  - Keep Explorer stable: no heavy clipboard parsing/conversion inside `explorer.exe`

### `PasteToFileHelper.exe` (out-of-proc)

- Location: `src/PasteToFileHelper`
- Responsibilities:
  - Read clipboard formats (text/HTML/RTF/bitmap)
  - Convert and write files to disk
  - Win+V clipboard history export via WinRT:
    - `Windows.ApplicationModel.DataTransfer.Clipboard::GetHistoryItemsAsync()`
  - Clear clipboard and history:
    - Win32: `EmptyClipboard()`
    - WinRT: `Clipboard::ClearHistory()`

### `PasteToFileCommon` (shared)

- Location: `src/PasteToFileCommon`
- Responsibilities:
  - Filename generation and collision avoidance
  - Clipboard format detection helpers
  - UTF helpers
  - Logging (`ptf.log`, `ptf-debug.log`)

## Data flow

1. Explorer calls the shell extension to populate the context menu.
2. The shell extension:
   - resolves a target directory
   - checks available clipboard formats
   - shows only relevant menu items
3. When the user clicks a menu item, the shell extension launches the helper EXE in the background.
4. The helper reads clipboard data and writes file(s) into the target directory.

## Why the helper EXE exists

Explorer loads in-proc shell extensions inside `explorer.exe`. Clipboard conversion, bitmap encoding,
WinRT history access, and file writes can be slow or error-prone; doing them in a helper process:

- reduces the risk of Explorer hangs/crashes
- simplifies WinRT usage
- makes testing easier (the helper can be exercised without Explorer)

