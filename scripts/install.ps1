param(
  [string]$BinDir = ""
)

$ErrorActionPreference = "Stop"

function Assert-Admin {
  $id = [Security.Principal.WindowsIdentity]::GetCurrent()
  $p = New-Object Security.Principal.WindowsPrincipal($id)
  if (-not $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Admin privileges required. Re-run PowerShell as Administrator."
  }
}

Assert-Admin

if ([string]::IsNullOrWhiteSpace($BinDir)) {
  # Default to typical VS output folder from repo root.
  $BinDir = Join-Path $PSScriptRoot "..\bin\x64\Release"
}

$BinDir = (Resolve-Path $BinDir).Path
$dll = Join-Path $BinDir "PasteToFileShellExt.dll"
$exe = Join-Path $BinDir "PasteToFileHelper.exe"

if (-not (Test-Path $dll)) { throw "Missing DLL: $dll" }
if (-not (Test-Path $exe)) { throw "Missing helper EXE: $exe (must be next to the DLL)" }

Write-Host "Registering COM server via regsvr32..."
& regsvr32.exe /s $dll

$clsid = "{4DFB17E1-9D8A-4E7D-8AA0-F1E8D1E3A6B7}"
$base = "HKLM:\Software\Classes"

$keys = @(
  "$base\AllFilesystemObjects\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\Background\shellex\ContextMenuHandlers\PasteToFile"
)

foreach ($k in $keys) {
  New-Item -Path $k -Force | Out-Null
  New-ItemProperty -Path $k -Name "(default)" -Value $clsid -PropertyType String -Force | Out-Null
}

Write-Host "Installed context menu handler."
Write-Host "You may need to restart Explorer (or sign out/in) to see the menu."
