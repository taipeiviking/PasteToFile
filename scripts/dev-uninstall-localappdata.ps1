param()

$ErrorActionPreference = "Stop"

function Assert-Admin {
  $id = [Security.Principal.WindowsIdentity]::GetCurrent()
  $p = New-Object Security.Principal.WindowsPrincipal($id)
  if (-not $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "Admin privileges required. Re-run PowerShell as Administrator."
  }
}

Assert-Admin

$dstDir = Join-Path $env:LOCALAPPDATA "PasteToFile"
$dll = Join-Path $dstDir "PasteToFileShellExt.dll"

$base = "HKLM:\Software\Classes"
$keys = @(
  "$base\AllFilesystemObjects\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\shellex\ContextMenuHandlers\PasteToFile",
  "$base\Directory\Background\shellex\ContextMenuHandlers\PasteToFile"
)

foreach ($k in $keys) {
  if (Test-Path $k) { Remove-Item -Path $k -Recurse -Force }
}

if (Test-Path $dll) {
  Write-Host "Unregistering COM server from $dll"
  & regsvr32.exe /s /u $dll
}

Write-Host "Dev-uninstalled handler keys. Restart Explorer to fully unload."
