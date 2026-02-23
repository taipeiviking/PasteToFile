param(
  [switch]$DeleteFiles
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

function Remove-KeyIfExists([string]$path) {
  if (Test-Path $path) {
    Remove-Item -Path $path -Recurse -Force
  }
}

function Unregister-ComDllIfPresent([string]$dllPath) {
  if (Test-Path $dllPath) {
    Write-Host "regsvr32 /u: $dllPath"
    try {
      & regsvr32.exe /s /u $dllPath
    } catch {
      Write-Host "Warning: regsvr32 /u failed: $($_.Exception.Message)"
    }
  }
}

Write-Host "PasteToFile reset starting..."

# 1) Remove Win11-primary MSIX package (if installed).
$pkg = Get-AppxPackage -Name "PasteToFile.Win11ContextMenu" -ErrorAction SilentlyContinue
if ($pkg) {
  Write-Host "Removing AppX package: $($pkg.PackageFullName)"
  try {
    Remove-AppxPackage -Package $pkg.PackageFullName
  } catch {
    Write-Host "Warning: Remove-AppxPackage failed: $($_.Exception.Message)"
  }
} else {
  Write-Host "AppX package not installed: PasteToFile.Win11ContextMenu"
}

# 2) Remove classic context menu handler registrations (both scopes).
$handlerSubkeys = @(
  "Software\Classes\AllFilesystemObjects\shellex\ContextMenuHandlers\PasteToFile",
  "Software\Classes\Directory\shellex\ContextMenuHandlers\PasteToFile",
  "Software\Classes\Directory\Background\shellex\ContextMenuHandlers\PasteToFile"
)

foreach ($k in $handlerSubkeys) {
  Remove-KeyIfExists ("HKLM:\{0}" -f $k)
  Remove-KeyIfExists ("HKCU:\{0}" -f $k)
}

# 3) Remove COM class registration (both scopes).
$clsid = "{4DFB17E1-9D8A-4E7D-8AA0-F1E8D1E3A6B7}"
Remove-KeyIfExists ("HKLM:\Software\Classes\CLSID\{0}" -f $clsid)
Remove-KeyIfExists ("HKCU:\Software\Classes\CLSID\{0}" -f $clsid)

# 4) Remove Approved entry (per-machine only).
$approvedKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (Test-Path $approvedKey) {
  try {
    $p = Get-ItemProperty -Path $approvedKey -Name $clsid -ErrorAction SilentlyContinue
    if ($null -ne $p) {
      Remove-ItemProperty -Path $approvedKey -Name $clsid -Force
      Write-Host "Removed Approved entry: $clsid"
    }
  } catch {
    Write-Host "Warning: could not remove Approved entry: $($_.Exception.Message)"
  }
}

# 5) Best-effort COM unregister for common deployment locations.
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$candidateDlls = @(
  (Join-Path $repoRoot "bin\x64\Release\PasteToFileShellExt.dll"),
  (Join-Path $env:LOCALAPPDATA "PasteToFile\PasteToFileShellExt.dll"),
  (Join-Path ${env:ProgramFiles} "PasteToFile\PasteToFileShellExt.dll"),
  (Join-Path ${env:ProgramFiles(x86)} "PasteToFile\PasteToFileShellExt.dll")
)
foreach ($dll in $candidateDlls) { Unregister-ComDllIfPresent $dll }

# 6) Optional file cleanup.
if ($DeleteFiles) {
  $paths = @(
    (Join-Path $env:LOCALAPPDATA "PasteToFile"),
    (Join-Path ${env:ProgramFiles} "PasteToFile"),
    (Join-Path ${env:ProgramFiles(x86)} "PasteToFile")
  )
  foreach ($p in $paths) {
    if (Test-Path $p) {
      Write-Host "Deleting: $p"
      try { Remove-Item -Path $p -Recurse -Force } catch { Write-Host "Warning: delete failed: $($_.Exception.Message)" }
    }
  }
} else {
  Write-Host "DeleteFiles not set; leaving installed folders/logs in place."
}

# 7) Restart Explorer so it unloads any in-proc DLL.
Write-Host "Restarting Explorer..."
Get-Process explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 600
Start-Process explorer.exe | Out-Null

Write-Host "PasteToFile reset complete."

