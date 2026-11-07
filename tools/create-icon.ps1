# Recreates the bundled original vector mark as a multi-size Windows icon.
param([string]$OutputPath = (Join-Path $PSScriptRoot '..\assets\veyra.ico'))
Add-Type -AssemblyName System.Drawing
$sizes = @(16,24,32,48,64,128,256)
$images = @()
foreach ($size in $sizes) {
    $bitmap = New-Object System.Drawing.Bitmap($size,$size)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.ScaleTransform(($size/256.0),($size/256.0))
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc(8,8,120,120,180,90)
    $path.AddArc(128,8,120,120,270,90)
    $path.AddArc(128,128,120,120,0,90)
    $path.AddArc(8,128,120,120,90,90)
    $path.CloseFigure()
    $background = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml('#1b142a'))
    $graphics.FillPath($background,$path)
    foreach ($shape in @(
        @{Color='#b99cff';XY=@(52,65,95,65,129,154,106,205)},
        @{Color='#eee7ff';XY=@(160,65,204,65,130,205,106,205)},
        @{Color='#9567ef';XY=@(176,65,204,65,182,107,154,107)}
    )) {
        $points = New-Object 'System.Drawing.PointF[]' ($shape.XY.Count/2)
        for ($i=0; $i -lt $points.Length; $i++) { $points[$i]=New-Object System.Drawing.PointF($shape.XY[$i*2],$shape.XY[$i*2+1]) }
        $brush=New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($shape.Color))
        $graphics.FillPolygon($brush,$points)
        $brush.Dispose()
    }
    $stream=New-Object System.IO.MemoryStream
    $bitmap.Save($stream,[System.Drawing.Imaging.ImageFormat]::Png)
    $images += ,($stream.ToArray())
    $stream.Dispose();$background.Dispose();$path.Dispose();$graphics.Dispose();$bitmap.Dispose()
}
$file=[System.IO.File]::Create([System.IO.Path]::GetFullPath($OutputPath))
$writer=New-Object System.IO.BinaryWriter($file)
try {
    $writer.Write([UInt16]0);$writer.Write([UInt16]1);$writer.Write([UInt16]$sizes.Count)
    $offset=6+16*$sizes.Count
    for ($i=0;$i -lt $sizes.Count;$i++) {
        $dimension=if($sizes[$i] -eq 256){0}else{$sizes[$i]}
        $writer.Write([byte]$dimension);$writer.Write([byte]$dimension)
        $writer.Write([UInt16]0);$writer.Write([UInt16]1);$writer.Write([UInt16]32)
        $writer.Write([UInt32]$images[$i].Length);$writer.Write([UInt32]$offset)
        $offset += $images[$i].Length
    }
    foreach($bytes in $images){$writer.Write([byte[]]$bytes)}
} finally {$writer.Dispose();$file.Dispose()}
Write-Output "Created multi-size icon: $OutputPath"
