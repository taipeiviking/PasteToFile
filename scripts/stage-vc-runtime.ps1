param(
  [string]$Platform = "x64",
  [string]$Configuration = "Release",
  [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

function Info([string]$msg) { Write-Host $msg }

function Get-RepoRoot {
  return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Find-VsWhere {
  $p = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $p) { return $p }
  return $null
}

function Find-CrtRedistDir([string]$platform) {
  $vswhere = Find-VsWhere
  if (-not $vswhere) { return $null }

  # Example match:
  # VC\Redist\MSVC\14.29.30037\x64\Microsoft.VC142.CRT\
  $pattern = if ($platform -ieq "x64") {
    "VC\\Redist\\MSVC\\**\\x64\\Microsoft.VC*.CRT\\"
  } else {
    throw "Unsupported platform: $platform"
  }

  $matches = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find $pattern
  if (-not $matches) { return $null }
  return ($matches | Select-Object -First 1)
}

$crtDir = Find-CrtRedistDir $Platform
function Get-FallbackSystem32Dir {
  # When invoked from a 32-bit process (e.g., 32-bit MSBuild), "System32" is file-redirected to SysWOW64.
  # "Sysnative" is the escape hatch to access the real 64-bit System32.
  $sysnative = Join-Path $env:WINDIR "Sysnative"
  if (Test-Path $sysnative) { return $sysnative }
  return (Join-Path $env:WINDIR "System32")
}

$root = Get-RepoRoot
$dest = $OutDir
if ([string]::IsNullOrWhiteSpace($dest)) {
  $dest = Join-Path $root ("bin\\{0}\\{1}" -f $Platform, $Configuration)
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null

$dlls = @("msvcp140.dll","vcruntime140.dll","vcruntime140_1.dll","concrt140.dll")

function Stage-FromDir([string]$fromDir) {
  Info "Staging MSVC runtime DLLs"
  Info "  From: $fromDir"
  Info "  To:   $dest"
  foreach ($name in $dlls) {
    $src = Join-Path $fromDir $name
    if (-not (Test-Path $src)) {
      throw "Missing runtime DLL: $src"
    }
    Copy-Item -Force $src (Join-Path $dest $name)
  }
}

if ($crtDir) {
  Stage-FromDir $crtDir
} else {
  # Some VS installs omit the "VC\Redist\MSVC\..." layout. Fall back to the system-installed runtime.
  $sys = Get-FallbackSystem32Dir
  Stage-FromDir $sys
}

Info "OK: staged MSVC runtime DLLs"

