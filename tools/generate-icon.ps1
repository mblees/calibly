# Generates the app icon resources/icon/calibly.ico (and calibly.png, 256 px):
# a blue book (reading) wearing orange headphones (listening) on the dark
# background of calibly. The .ico holds PNG images of the sizes Windows uses
# (16 - 256 px); each size is drawn on its own, so small sizes stay sharp.
# Run from the project folder:
#   powershell -ExecutionPolicy Bypass -File tools/generate-icon.ps1

Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot "..\resources\icon"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function Color([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }

function RoundedRect([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    return $path
}

# Draws the icon on a 256 x 256 grid, scaled to $size pixels.
function Draw-Icon([int]$size) {
    $bitmap = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bitmap)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.ScaleTransform($size / 256.0, $size / 256.0)

    # Background
    $background = RoundedRect 8 8 240 240 52
    $fill = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.PointF 0, 8), (New-Object System.Drawing.PointF 0, 248), (Color "#2a2b32"), (Color "#121316")
    $g.FillPath($fill, $background)

    # Book (2:3) with a darker spine
    $book = RoundedRect 86 88 84 126 9
    $bookFill = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.PointF 0, 88), (New-Object System.Drawing.PointF 0, 214), (Color "#8bbcff"), (Color "#4a82e0")
    $g.FillPath($bookFill, $book)
    if ($size -ge 32) {
        $spine = New-Object System.Drawing.SolidBrush (Color "#3466c0")
        $g.FillRectangle($spine, 86 + 9, 88, 12, 126)
        $lines = New-Object System.Drawing.Pen (Color "#d8e7ff"), 6
        $lines.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
        $lines.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
        $g.DrawLine($lines, 122, 124, 154, 124)
        $g.DrawLine($lines, 122, 140, 146, 140)
    }

    # Headphones: band over the book, ear cups at its sides
    $orange = Color "#ff8a3d"
    $band = New-Object System.Drawing.Pen $orange, 16
    $band.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $band.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawArc($band, 55, 50, 146, 146, 180, 180)
    $cups = New-Object System.Drawing.SolidBrush $orange
    $g.FillPath($cups, (RoundedRect 40 118 36 66 14))
    $g.FillPath($cups, (RoundedRect 180 118 36 66 14))

    $g.Dispose()
    return $bitmap
}

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$images = @()
foreach ($size in $sizes) {
    $bitmap = Draw-Icon $size
    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $images += , $stream.ToArray()
    if ($size -eq 256) { $bitmap.Save((Join-Path $outDir "calibly.png"), [System.Drawing.Imaging.ImageFormat]::Png) }
    $bitmap.Dispose()
}

# ICO file: header, one directory entry per image, then the PNG data.
$file = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter $file
$writer.Write([UInt16]0)              # reserved
$writer.Write([UInt16]1)              # type: icon
$writer.Write([UInt16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $edge = if ($sizes[$i] -ge 256) { 0 } else { $sizes[$i] }  # 0 means 256
    $writer.Write([byte]$edge)            # width
    $writer.Write([byte]$edge)            # height
    $writer.Write([byte]0)                # colors in palette
    $writer.Write([byte]0)                # reserved
    $writer.Write([UInt16]1)              # color planes
    $writer.Write([UInt16]32)             # bits per pixel
    $writer.Write([UInt32]$images[$i].Length)
    $writer.Write([UInt32]$offset)
    $offset += $images[$i].Length
}
foreach ($image in $images) { $writer.Write($image) }
$writer.Flush()
[System.IO.File]::WriteAllBytes((Join-Path $outDir "calibly.ico"), $file.ToArray())
Write-Host "Written: $outDir\calibly.ico, calibly.png"
