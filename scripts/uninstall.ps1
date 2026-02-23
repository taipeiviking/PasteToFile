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
  $BinDir = Join-Path $PSScriptRoot "..\bin\x64\Release"
}

$BinDir = (Resolve-Path $BinDir).Path
$dll = Join-Path $BinDir "PasteToFileShellExt.dll"

$base = "HKLM:\Software\Classes"
$keys = @(
  "$base\AllFilesystemObjects\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\Background\shellex\ContextMenuHandlers\PasteToFile"
)

foreach ($k in $keys) {
  if (Test-Path $k) {
    Remove-Item -Path $k -Recurse -Force
  }
}

if (Test-Path $dll) {
  Write-Host "Unregistering COM server via regsvr32..."
  & regsvr32.exe /s /u $dll
} else {
  Write-Host "DLL not found at $dll; skipping regsvr32 /u"
}

Write-Host "Uninstalled context menu handler."
Write-Host "You may need to restart Explorer (or sign out/in)."
