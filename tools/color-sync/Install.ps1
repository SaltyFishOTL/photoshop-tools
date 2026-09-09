$ErrorActionPreference='Stop'
$scripts='C:\Program Files\Adobe\Adobe Photoshop 2025\Presets\Scripts'
$data=Join-Path $env:APPDATA 'ColorSync'
if(!(Test-Path -LiteralPath $scripts)){throw 'Photoshop 2025 script directory not found'}
$backup=Join-Path $data ('Backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
foreach($file in @('ColorSyncLauncher.exe','ColorSync.psjs')){if(Test-Path -LiteralPath (Join-Path $data $file)){Copy-Item -LiteralPath (Join-Path $data $file) -Destination $backup}}
if(Test-Path -LiteralPath (Join-Path $scripts 'ColorSync.jsx')){Copy-Item -LiteralPath (Join-Path $scripts 'ColorSync.jsx') -Destination $backup}
New-Item -ItemType Directory -Path $data -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ColorSyncLauncher.exe') -Destination $data -Force
$payload=Join-Path $PSScriptRoot 'ColorSync.psjs'
Copy-Item -LiteralPath $payload -Destination (Join-Path $data 'ColorSync.psjs') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ColorSync.jsx') -Destination $scripts -Force
foreach($file in @('ColorSyncLauncher.exe','ColorSync.psjs')){if((Get-FileHash -LiteralPath (Join-Path $data $file)).Hash -ne (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $file)).Hash){throw ('Installed hash mismatch: '+$file)}}
@{data=$data;backup=$backup;payload=(Get-FileHash -LiteralPath (Join-Path $data 'ColorSync.psjs')).Hash;menu=(Get-FileHash -LiteralPath (Join-Path $scripts 'ColorSync.jsx')).Hash} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'install-result.json') -Encoding UTF8
