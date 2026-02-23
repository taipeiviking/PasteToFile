param(
  [string]$Configuration = "Release",
  [string]$Platform = "x64"
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

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$bin = Join-Path $root ("bin\{0}\{1}" -f $Platform, $Configuration)

$srcDll = Join-Path $bin "PasteToFileShellExt.dll"
$srcExe = Join-Path $bin "PasteToFileHelper.exe"
if (-not (Test-Path $srcDll)) { throw "Missing: $srcDll (build first)" }
if (-not (Test-Path $srcExe)) { throw "Missing: $srcExe (build first)" }

$dstDir = Join-Path $env:LOCALAPPDATA "PasteToFile"
New-Item -ItemType Directory -Force -Path $dstDir | Out-Null

$dstDll = Join-Path $dstDir "PasteToFileShellExt.dll"
$dstExe = Join-Path $dstDir "PasteToFileHelper.exe"
$dbgLog = Join-Path $dstDir "ptf-debug.log"

Write-Host "Stopping Explorer..."
Get-Process explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

Write-Host "Copying binaries to $dstDir"
Copy-Item -Force $srcDll $dstDll
Copy-Item -Force $srcExe $dstExe

Write-Host "Registering COM server: $dstDll"
& regsvr32.exe /s $dstDll

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

Write-Host "Starting Explorer..."
Start-Process explorer.exe | Out-Null

Write-Host ""
Write-Host "Done."
Write-Host "Now right-click any file/folder and choose PasteToFile."
Write-Host "Debug log should be written here:"
Write-Host "  $dbgLog"
