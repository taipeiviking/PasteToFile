param(
  [string]$Configuration = "Release",
  [string]$Platform = "x64",
  [string]$OutDir = "",
  [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

function Assert-True([bool]$cond, [string]$msg) {
  if (-not $cond) { throw "ASSERT FAILED: $msg" }
}

function Info([string]$msg) { Write-Host $msg }

function Get-RepoRoot {
  return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-HelperPath([string]$root, [string]$platform, [string]$config) {
  return Join-Path $root ("bin\{0}\{1}\PasteToFileHelper.exe" -f $platform, $config)
}

function New-TestDir([string]$root) {
  if (-not [string]::IsNullOrWhiteSpace($OutDir)) {
    $p = (Resolve-Path $OutDir).Path
    New-Item -ItemType Directory -Force -Path $p | Out-Null
    return $p
  }
  $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $p = Join-Path $root ("_autotest\run-{0}" -f $stamp)
  New-Item -ItemType Directory -Force -Path $p | Out-Null
  return $p
}

function LatestPtfFiles([string]$dir) {
  Get-ChildItem -Path $dir -File -Filter "PTF-*" -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTimeUtc, Name
}

function LatestPtfFileByExt([string]$dir, [string]$ext) {
  LatestPtfFiles $dir | Where-Object { $_.Extension -ieq $ext } | Select-Object -Last 1
}

function Ensure-STA {
  # Clipboard APIs are easiest in STA.
  $state = [System.Threading.Thread]::CurrentThread.ApartmentState
  if ($state -ne "STA") {
    throw "This script must be run in STA mode. Re-run with: powershell.exe -STA -File scripts\test-helper.ps1"
  }
}

function Set-ClipboardText([string]$text) {
  Set-Clipboard -Value $text
}

function Build-HtmlClipboardFormat([string]$htmlFragment) {
  # Minimal CF_HTML payload with correct byte offsets.
  # See https://learn.microsoft.com/en-us/windows/win32/dataxchg/html-clipboard-format
  $startMarker = "<!--StartFragment-->"
  $endMarker = "<!--EndFragment-->"
  $html = "<html><body>$startMarker$htmlFragment$endMarker</body></html>"

  $headerTemplate =
    "Version:0.9`r`n" +
    "StartHTML:0000000000`r`n" +
    "EndHTML:0000000000`r`n" +
    "StartFragment:0000000000`r`n" +
    "EndFragment:0000000000`r`n"

  $startHtml = $headerTemplate.Length
  $endHtml = $startHtml + $html.Length

  $fragStartInHtml = $html.IndexOf($startMarker) + $startMarker.Length
  $fragEndInHtml = $html.IndexOf($endMarker)
  Assert-True ($fragStartInHtml -ge 0) "HTML fragment start marker not found"
  Assert-True ($fragEndInHtml -ge 0) "HTML fragment end marker not found"

  $startFragment = $startHtml + $fragStartInHtml
  $endFragment = $startHtml + $fragEndInHtml

  $pad10 = { param([int]$n) $n.ToString("D10") }
  $header =
    "Version:0.9`r`n" +
    ("StartHTML:{0}`r`n" -f (& $pad10 $startHtml)) +
    ("EndHTML:{0}`r`n" -f (& $pad10 $endHtml)) +
    ("StartFragment:{0}`r`n" -f (& $pad10 $startFragment)) +
    ("EndFragment:{0}`r`n" -f (& $pad10 $endFragment))

  return $header + $html
}

function Set-ClipboardRich([string]$plainText, [string]$rtf, [string]$htmlClip) {
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  $data = New-Object System.Windows.Forms.DataObject
  if ($plainText) { $data.SetText($plainText, [System.Windows.Forms.TextDataFormat]::UnicodeText) }
  if ($rtf) { $data.SetData("Rich Text Format", $rtf) }
  if ($htmlClip) { $data.SetData("HTML Format", $htmlClip) }
  [System.Windows.Forms.Clipboard]::SetDataObject($data, $true)
}

function Set-ClipboardTestImage {
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  Add-Type -AssemblyName System.Drawing | Out-Null

  $bmp = New-Object System.Drawing.Bitmap 32, 32
  for ($y = 0; $y -lt 32; $y++) {
    for ($x = 0; $x -lt 32; $x++) {
      $c = if ($x -eq 0 -and $y -eq 0) { [System.Drawing.Color]::Red } else { [System.Drawing.Color]::Blue }
      $bmp.SetPixel($x, $y, $c)
    }
  }
  [System.Windows.Forms.Clipboard]::SetImage($bmp)
  $bmp.Dispose()
}

function Run-Helper([string]$helper, [string]$dir, [string]$action) {
  & $helper --target "$dir" --action $action | Out-Null
  Assert-True ($LASTEXITCODE -eq 0) "Helper exited with $LASTEXITCODE for action=$action"
}

function Verify-TextFile([string]$dir, [string]$ext, [string]$expected) {
  $f = LatestPtfFileByExt $dir $ext
  Assert-True ($null -ne $f) "Expected a $ext file to be created"
  $got = Get-Content -Path $f.FullName -Raw -Encoding UTF8
  Assert-True ($got -eq $expected) "Content mismatch for $($f.Name)"
  Info "OK: $($f.Name) content matches"
}

function Verify-StartsWith([string]$dir, [string]$ext, [string]$prefix) {
  $f = LatestPtfFileByExt $dir $ext
  Assert-True ($null -ne $f) "Expected a $ext file to be created"
  $got = Get-Content -Path $f.FullName -Raw -Encoding UTF8
  Assert-True ($got.StartsWith($prefix)) "Expected $($f.Name) to start with '$prefix'"
  Info "OK: $($f.Name) starts with '$prefix'"
}

function Verify-Png([string]$dir) {
  Add-Type -AssemblyName System.Drawing | Out-Null
  $f = LatestPtfFileByExt $dir ".png"
  Assert-True ($null -ne $f) "Expected a .png file to be created"
  $img = [System.Drawing.Bitmap]::FromFile($f.FullName)
  try {
    Assert-True ($img.Width -eq 32 -and $img.Height -eq 32) "Unexpected PNG size: $($img.Width)x$($img.Height)"
    $p00 = $img.GetPixel(0, 0)
    Assert-True ($p00.R -gt 200 -and $p00.G -lt 50 -and $p00.B -lt 50) "Top-left pixel is not red as expected"
  } finally {
    $img.Dispose()
  }
  Info "OK: $($f.Name) PNG decoded and matches expected pixel"
}

Ensure-STA

$root = Get-RepoRoot
$helper = Get-HelperPath $root $Platform $Configuration
Assert-True (Test-Path $helper) "Helper not found: $helper (build Release|x64 first)"

$testDir = New-TestDir $root
Info "TestDir: $testDir"

# Force debug log next to the helper for easier inspection.
$env:PTF_LOG_DIR = (Split-Path -Parent $helper)

Info "Helper: $helper"
Info "DebugLogDir: $env:PTF_LOG_DIR"

Info "== Test 1: Text (.txt) =="
$t1 = "Hello PasteToFile test (text)"
Set-ClipboardText $t1
Run-Helper $helper $testDir "text-txt"
Verify-TextFile $testDir ".txt" $t1

Info "== Test 2: Collision suffix (-01) =="
$t2 = "Second write collision test"
Set-ClipboardText $t2
Run-Helper $helper $testDir "text-txt"
$fTxt = LatestPtfFileByExt $testDir ".txt"
Assert-True ($fTxt.Name -match "-01\.txt$") "Expected collision-suffixed file, got $($fTxt.Name)"
Verify-TextFile $testDir ".txt" $t2

Info "== Test 3: Markdown (.md) =="
$md = "# Title`n`n- item1`n"
Set-ClipboardText $md
Run-Helper $helper $testDir "text-md"
Verify-TextFile $testDir ".md" $md

Info "== Test 4: Image (PNG) =="
Set-ClipboardTestImage
Run-Helper $helper $testDir "png"
Verify-Png $testDir

Info "== Test 5: HTML (.html) =="
$htmlClip = Build-HtmlClipboardFormat "<b>Bold</b> and <i>italic</i>"
Set-ClipboardRich "Plain fallback" $null $htmlClip
Run-Helper $helper $testDir "html"
Verify-StartsWith $testDir ".html" "<html>"

Info "== Test 6: RTF (.rtf) =="
$rtf = "{\rtf1\ansi\deff0 {\fonttbl{\f0 Arial;}} \f0\fs24 Hello \b RTF\b0 }"
Set-ClipboardRich "Plain fallback" $rtf $null
Run-Helper $helper $testDir "rtf"
Verify-StartsWith $testDir ".rtf" "{\rtf1"

Info "== Test 7: Save All Available Formats (txt/html/rtf) =="
$htmlClip2 = Build-HtmlClipboardFormat "Fragment2"
Set-ClipboardRich "SaveAll text" $rtf $htmlClip2
Run-Helper $helper $testDir "all"
Assert-True ((LatestPtfFiles $testDir | Where-Object Extension -ieq ".txt").Count -ge 3) "Expected additional .txt from SaveAll"
Assert-True ((LatestPtfFiles $testDir | Where-Object Extension -ieq ".html").Count -ge 2) "Expected additional .html from SaveAll"
Assert-True ((LatestPtfFiles $testDir | Where-Object Extension -ieq ".rtf").Count -ge 2) "Expected additional .rtf from SaveAll"
Info "OK: SaveAll produced multiple files"

Info ""
Info "ALL TESTS PASSED"
Info "Outputs: $testDir"
Info "Debug log: $env:PTF_LOG_DIR\\ptf-debug.log"

if (-not $KeepOutput) {
  Info "Tip: re-run with -KeepOutput to keep artifacts."
}
