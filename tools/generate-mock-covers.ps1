# Generates the sample cover images in resources/mock/covers.
# Book covers are 2:3, audiobook covers are square (like real audiobook covers).
# Non-ASCII letters are written as [char] codes, so Windows PowerShell 5.1
# reads this file correctly without a BOM.
# Run from the project folder:
#   powershell -ExecutionPolicy Bypass -File tools/generate-mock-covers.ps1

Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot "..\resources\mock\covers"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# file, title, author, top color, bottom color, square
$covers = @(
    @("moby-dick",           "Moby Dick",              "Herman Melville",   "#274b6d", "#0f2236", $false),
    @("faust",               "Faust I",                "J. W. von Goethe",  "#5c3f78", "#25133a", $false),
    @("effi-briest",         "Effi Briest",            "Theodor Fontane",   "#3f6b4a", "#16301d", $false),
    @("krieg-und-frieden",   "Krieg und Frieden",      "Leo Tolstoi",       "#9b3d2a", "#4a160c", $false),
    @("die-verwandlung",     "Die Verwandlung",        "Franz Kafka",       "#7a6a50", "#352b1b", $false),
    @("buddenbrooks",        "Buddenbrooks",           "Thomas Mann",       "#c46a2b", "#5d2a07", $false),
    @("grimms-maerchen",     "Grimms M$([char]0xE4)rchen",         "Br$([char]0xFC)der Grimm",      "#c49a3a", "#6e4c10", $false),
    @("robinson-crusoe",     "Robinson Crusoe",        "Daniel Defoe",      "#3a4f9c", "#141e4d", $false),
    @("woyzeck",             "Woyzeck",                "Georg B$([char]0xFC)chner",     "#575d66", "#1f2328", $false),
    @("die-drei-musketiere", "Die drei Musketiere",    "Alexandre Dumas",   "#2f6f73", "#0d3234", $false),
    @("dracula",             "Dracula",                "Bram Stoker",       "#a8475f", "#4d1424", $true),
    @("frankenstein",        "Frankenstein",           "Mary Shelley",      "#2f6f73", "#0d3234", $true),
    @("die-schatzinsel",     "Die Schatzinsel",        "R. L. Stevenson",   "#4d7e8f", "#173743", $true),
    @("der-zauberer-von-oz", "Der Zauberer von Oz",    "L. Frank Baum",     "#c46a2b", "#5d2a07", $true)
)

$jpeg = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$params = New-Object System.Drawing.Imaging.EncoderParameters 1
$params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), 88L

foreach ($c in $covers) {
    $file, $title, $author, $top, $bottom, $square = $c
    $w = 480
    $h = if ($square) { 480 } else { 720 }

    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = "AntiAlias"
    $g.TextRenderingHint = "AntiAliasGridFit"

    $rect = New-Object System.Drawing.Rectangle 0, 0, $w, $h
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $rect,
        ([System.Drawing.ColorTranslator]::FromHtml($top)),
        ([System.Drawing.ColorTranslator]::FromHtml($bottom)), 70.0
    $g.FillRectangle($brush, $rect)

    # Decorative frame
    $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(70, 255, 255, 255)), 3
    $g.DrawRectangle($pen, 24, 24, $w - 48, $h - 48)

    $white = [System.Drawing.Brushes]::White
    $soft = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(210, 255, 255, 255))
    # Shrink the title font until the longest word fits on one line.
    $titleSize = if ($square) { 46 } else { 52 }
    $longestWord = @($title -split " " | Sort-Object Length -Descending)[0]
    do {
        $titleFont = New-Object System.Drawing.Font "Georgia", $titleSize, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
        $wordWidth = $g.MeasureString($longestWord, $titleFont).Width
        $titleSize -= 2
    } while ($wordWidth -gt ($w - 150) -and $titleSize -gt 20)
    $authorFont = New-Object System.Drawing.Font "Segoe UI", 24, ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)
    $smallFont = New-Object System.Drawing.Font "Segoe UI", 18, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)

    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = "Center"
    $titleRect = New-Object System.Drawing.RectangleF 48, $(if ($square) { 110 } else { 150 }), ($w - 96), 260
    $g.DrawString($title, $titleFont, $white, $titleRect, $fmt)

    $authorRect = New-Object System.Drawing.RectangleF 48, ($h - 110), ($w - 96), 40
    $g.DrawString($author, $authorFont, $soft, $authorRect, $fmt)

    if ($square) {
        $labelRect = New-Object System.Drawing.RectangleF 48, 52, ($w - 96), 30
        $g.DrawString("H$([char]0xD6)RBUCH $([char]0xB7) UNGEK$([char]0xDC)RZT", $smallFont, $soft, $labelRect, $fmt)
    }

    $path = Join-Path $outDir "$file.jpg"
    $bmp.Save($path, $jpeg, $params)
    $g.Dispose(); $bmp.Dispose()
    Write-Output "wrote $path"
}
