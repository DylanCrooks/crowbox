# Uploads the vision firmware to the box over Wi-Fi, then confirms it (see wifi_ota.c).
#
#   1. Build: idf.py build (from vision/).
#   2. Power-cycle the box. The update window opens on power-on, not on deep-sleep wakes, and
#      closes ~3 minutes after boot if no laptop joins.
#   3. Join the "crowbox-vision" Wi-Fi (password in wifi_ota_secrets.h). Tick "Connect
#      automatically" so the laptop rejoins when the board reboots into the new firmware.
#   4. From vision/:  .\components\wifi_ota\ota_upload.ps1
#
# Status page at any time while the window is open: http://192.168.4.1/
param(
    [string]$Bin = "$PSScriptRoot\..\..\build\vision.bin",
    [string]$Address = "192.168.4.1",
    [int]$ConfirmTimeoutS = 240     # the board rolls back if not confirmed within 300 s
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Bin)) { throw "No $Bin. Build first (idf.py build)." }
$bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $Bin))
# esp_app_desc_t starts at byte 32 of an app image: version at +16, project_name at +48.
$version = [System.Text.Encoding]::ASCII.GetString($bytes, 48, 32).Trim([char]0)
$project = [System.Text.Encoding]::ASCII.GetString($bytes, 80, 32).Trim([char]0)
if ($project -ne 'vision') { throw "$Bin is the '$project' project, not the vision firmware." }

Write-Host ("Uploading {0} ({1:N2} MB) to http://{2}/update ..." -f $version, ($bytes.Length / 1MB), $Address)
$reply = (curl.exe --fail-with-body --silent --show-error --max-time 300 `
    -H "Content-Type: application/octet-stream" --data-binary "@$Bin" "http://$Address/update") -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "Upload failed (curl exit $LASTEXITCODE): $reply`nThe board keeps its current firmware."
}
Write-Host $reply
if ($reply -notmatch 'to (ota_\d)') { throw "Unexpected reply from the board." }
$slot = $Matches[1]

# The board reboots ~1 s after replying. The first request to reach the new firmware confirms it.
Write-Host "Waiting for the board to come back in $slot (laptop must rejoin crowbox-vision) ..."
Start-Sleep -Seconds 5
$deadline = (Get-Date).AddSeconds($ConfirmTimeoutS)
while ((Get-Date) -lt $deadline) {
    $status = (curl.exe --silent --max-time 3 "http://$Address/status") -join "`n"
    if ($LASTEXITCODE -eq 0 -and $status -match 'running:\s+(ota_\d)') {
        Write-Host "`n$status"
        if ($Matches[1] -eq $slot) {
            Write-Host "Update confirmed: the board runs $version from $slot." -ForegroundColor Green
            exit 0
        }
        Write-Host "The board came back in $($Matches[1]): the new firmware failed its boot and rolled back." -ForegroundColor Red
        exit 1
    }
    Start-Sleep -Seconds 2
}
Write-Host "Couldn't reach the board within $ConfirmTimeoutS s. If it doesn't hear from a laptop within 300 s of rebooting, it rolls back to the previous firmware." -ForegroundColor Yellow
exit 1
