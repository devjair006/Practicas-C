$files = @("assets\puertadere.obj", "assets\puertaizqui.obj")
foreach ($f in $files) {
    $lines = Get-Content $f
    $xs = @()
    $ys = @()
    $zs = @()
    foreach ($l in $lines) {
        if ($l -match '^v ') {
            $parts = $l.Trim() -split '\s+'
            $xs += [float]$parts[1]
            $ys += [float]$parts[2]
            $zs += [float]$parts[3]
        }
    }
    $xmin = ($xs | Measure-Object -Minimum).Minimum
    $xmax = ($xs | Measure-Object -Maximum).Maximum
    $ymin = ($ys | Measure-Object -Minimum).Minimum
    $ymax = ($ys | Measure-Object -Maximum).Maximum
    $zmin = ($zs | Measure-Object -Minimum).Minimum
    $zmax = ($zs | Measure-Object -Maximum).Maximum
    Write-Host "=== $f ==="
    Write-Host "X: $xmin to $xmax (width: $($xmax - $xmin))"
    Write-Host "Y: $ymin to $ymax (height: $($ymax - $ymin))"
    Write-Host "Z: $zmin to $zmax (depth: $($zmax - $zmin))"
    Write-Host ""
}
