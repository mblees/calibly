# Generates the app icon resources/icon/calibly.ico (and calibly.png, 256 px):
# book spines on a shelf whose heights form an equalizer, blue (reading) and
# orange (listening) in turn, on the dark background of calibly (idea 4 of
# mockups/calibly-logo-ideen.html). The .ico holds PNG images of the sizes
# Windows uses (16 - 256 px); each size is drawn on its own, and sizes below
# 48 px use a simpler drawing, so small sizes stay sharp.
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

    $orange = New-Object System.Drawing.SolidBrush (Color "#ff8a3d")

    if ($size -ge 48) {
        # Five spines on a shelf. Blue spines carry a vertical gradient and a
        # pale band like a printed spine; the bar heights form an equalizer.
        $shelf = New-Object System.Drawing.SolidBrush (Color "#4a4b55")
        $g.FillPath($shelf, (RoundedRect 37 196 182 10 5))
        $band = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(115, (Color "#d8e7ff")))
        # x, y (top), height of each spine; all are 26 wide and stand on y = 192
        foreach ($spine in @(@(47, 116, 76, "blue"), @(81, 68, 124, "orange"), @(115, 96, 96, "blue"),
                             @(149, 52, 140, "orange"), @(183, 128, 64, "blue"))) {
            $x = $spine[0]; $y = $spine[1]; $h = $spine[2]
            $shape = RoundedRect $x $y 26 $h 5
            if ($spine[3] -eq "orange") {
                $g.FillPath($orange, $shape)
            } else {
                # The gradient runs 1 unit past the spine: GDI+ wraps the
                # first color around at the exact end point.
                $blue = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.PointF 0, ($y - 1)), (New-Object System.Drawing.PointF 0, ($y + $h + 1)), (Color "#8bbcff"), (Color "#4a82e0")
                $g.FillPath($blue, $shape)
                $g.FillRectangle($band, $x, $y + 14, 26, 6)
            }
        }
    } else {
        # Small sizes: four wider spines without shelf, bands and gradient,
        # so each spine keeps at least two pixels at 16 px.
        $blue = New-Object System.Drawing.SolidBrush (Color "#6ea8ff")
        $g.FillPath($blue, (RoundedRect 40 110 38 98 8))
        $g.FillPath($orange, (RoundedRect 88 52 38 156 8))
        $g.FillPath($blue, (RoundedRect 136 86 38 122 8))
        $g.FillPath($orange, (RoundedRect 184 132 34 76 8))
    }

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
