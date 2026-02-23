param(
  [string]$OutFile = "",
  [switch]$Force
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutFile)) {
  throw "OutFile is required. Example: -OutFile installer\\assets\\vc_redist.x64.exe"
}

$dest = (Resolve-Path (Split-Path -Parent $OutFile)).Path
New-Item -ItemType Directory -Force -Path $dest | Out-Null

$outPath = Join-Path $dest (Split-Path -Leaf $OutFile)
if ((-not $Force) -and (Test-Path $outPath)) {
  Write-Host "VC++ redist already present: $outPath"
  exit 0
}

# Official Microsoft "latest" link (may redirect).
$url = "https://aka.ms/vs/17/release/vc_redist.x64.exe"

Write-Host "Downloading VC++ redist..."
Write-Host "  From: $url"
Write-Host "  To:   $outPath"

try {
  Invoke-WebRequest -Uri $url -OutFile $outPath -UseBasicParsing
} catch {
  throw ("Failed to download vc_redist.x64.exe. " +
         "Make sure you have internet access, or manually download from: $url " +
         "and save it to: $outPath")
}

if (-not (Test-Path $outPath)) {
  throw "Download completed but file was not found: $outPath"
}

$len = (Get-Item $outPath).Length
if ($len -lt 1MB) {
  throw "Downloaded vc_redist.x64.exe looks too small ($len bytes). Delete it and retry."
}

Write-Host "OK: downloaded vc_redist.x64.exe ($len bytes)"

