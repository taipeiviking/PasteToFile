param(
  [string]$Template = "",
  [string]$Step1Png = "",
  [string]$Step2Png = "",
  [string]$OutRtf = ""
)

$ErrorActionPreference = "Stop"

function Resolve-OrDefault([string]$v, [string]$defaultRel) {
  if (![string]::IsNullOrWhiteSpace($v)) { return (Resolve-Path $v).Path }
  return (Resolve-Path (Join-Path $PSScriptRoot $defaultRel)).Path
}

$templatePath = Resolve-OrDefault $Template "..\\assets\\howto-template.rtf"
$step1Path = Resolve-OrDefault $Step1Png "..\\..\\images\\Menu1.png"
$step2Path = Resolve-OrDefault $Step2Png "..\\..\\images\\Menu2.png"
$outPath = if (![string]::IsNullOrWhiteSpace($OutRtf)) { (Resolve-Path $OutRtf).Path } else { (Join-Path (Split-Path $templatePath -Parent) "howto.rtf") }

if (!(Test-Path $templatePath)) { throw "Missing template: $templatePath" }
if (!(Test-Path $step1Path)) { throw "Missing step1 image: $step1Path" }
if (!(Test-Path $step2Path)) { throw "Missing step2 image: $step2Path" }

Add-Type -AssemblyName System.Drawing | Out-Null

function Build-RtfPictFromPng([string]$pngPath, [int]$maxWidth) {
  # Resize screenshot down so the embedded RTF stays small and doesn’t bloat the MSI.
  $img = [System.Drawing.Image]::FromFile($pngPath)
  try {
    $scale = [Math]::Min(1.0, $maxWidth / [double]$img.Width)
    $w = [int][Math]::Round($img.Width * $scale)
    $h = [int][Math]::Round($img.Height * $scale)

    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
      $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
      $g.DrawImage($img, 0, 0, $w, $h)
    } finally {
      $g.Dispose()
    }

    $ms = New-Object System.IO.MemoryStream
    try {
      $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
      $bytes = $ms.ToArray()
    } finally {
      $ms.Dispose()
      $bmp.Dispose()
    }
  } finally {
    $img.Dispose()
  }

  # RTF picture dimensions:
  # - \picw/\pich are pixels
  # - \picwgoal/\pichgoal are twips (1 inch = 1440 twips, 96 px/in => 15 twips/px)
  $twipsPerPx = 15
  $picwgoal = $w * $twipsPerPx
  $pichgoal = $h * $twipsPerPx

  # Hex encode bytes, no whitespace (RTF allows optional whitespace, but none keeps it simple).
  $hex = ($bytes | ForEach-Object { $_.ToString("x2") }) -join ""
  # Important: use single backslashes for RTF control words.
  return "{\pard\qc{\pict\pngblip\picw$w\pich$h\picwgoal$picwgoal\pichgoal$pichgoal " + $hex + "}\par}"
}

$maxWidth = 560
$pict1 = Build-RtfPictFromPng $step1Path $maxWidth
$pict2 = Build-RtfPictFromPng $step2Path $maxWidth

$templateText = Get-Content -Raw -Encoding ASCII $templatePath
if ($templateText -notmatch "__PTF_STEP1_PICT__") {
  throw "Template missing placeholder __PTF_STEP1_PICT__"
}
if ($templateText -notmatch "__PTF_STEP2_PICT__") {
  throw "Template missing placeholder __PTF_STEP2_PICT__"
}

$outText = $templateText.Replace("__PTF_STEP1_PICT__", $pict1)
$outText = $outText.Replace("__PTF_STEP2_PICT__", $pict2)
Set-Content -Encoding ASCII -Path $outPath -Value $outText

Write-Host "Generated: $outPath"

