# Development

## Prereqs

- Windows 10/11
- Visual Studio (Desktop development with C++)
- Build target: `x64`

## Build

- Open `PasteToFile.sln`
- Build `Debug|x64` or `Release|x64`

## Dev install scripts (recommended)

Shell extensions run inside `explorer.exe`. For reliable iteration, use the dev scripts:

- Install to `%LOCALAPPDATA%\\PasteToFile`:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-install-localappdata.ps1`
- Uninstall from `%LOCALAPPDATA%\\PasteToFile`:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-uninstall-localappdata.ps1`
- Reinstall + restart Explorer:
  - `powershell -ExecutionPolicy Bypass -File scripts\\dev-reinstall-and-restart-explorer.ps1`

## Testing without Explorer

The helper supports a test harness that:

- sets clipboard formats in PowerShell (STA)
- runs the helper actions
- validates outputs

Run:

- `powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File scripts\\test-helper.ps1 -Configuration Release -Platform x64 -KeepOutput`

## Debugging

Log files:

- `%LOCALAPPDATA%\\PasteToFile\\ptf.log`
- `ptf-debug.log` (see README for search order)

To force logs into a directory you control:

- set `PTF_LOG_DIR` (environment variable) before launching Explorer / the helper

Common failure signals:

- Helper launch failures: look for `[ShellExt] CreateProcess failed` in `ptf-debug.log`
- Target directory resolution failures: look for `[ShellExt] Initialize failed to resolve target dir`

