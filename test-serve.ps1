# Pipe-protocol smoke test for `FaceFXWrapper serve`.
# Build Release|Win32 first. Optional real LIP: -Fonix <cdf> -Wav <16khz.wav>
param(
    [string]$Exe = (Join-Path $PSScriptRoot 'x86\Release\FaceFXWrapper.exe'),
    [string]$Type = 'Fallout4',
    [string]$Fonix = '',
    [string]$Wav = '',
    [string]$Text = 'prihveet'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Exe)) {
    throw "Build the Win32 Release exe first: $Exe"
}

$lip = Join-Path $env:TEMP 'facefx-serve-test.lip'
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Exe
$psi.Arguments = "serve $Type"
$psi.WorkingDirectory = Split-Path $Exe
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true

$proc = [System.Diagnostics.Process]::Start($psi)
$ready = $false
$deadline = [datetime]::UtcNow.AddMinutes(2)

while (-not $ready -and [datetime]::UtcNow -lt $deadline) {
    $line = $proc.StandardOutput.ReadLine()
    if ($null -eq $line) { break }
    Write-Host $line
    if ($line.StartsWith('FXW READY')) { $ready = $true }
}

if (-not $ready) {
    $proc.Kill()
    throw 'Did not receive FXW READY (CK init failed or stdin is not connected)'
}

$proc.StandardInput.WriteLine('PING')
$reply = $proc.StandardOutput.ReadLine()
Write-Host $reply
if ($reply -ne 'FXW ERR unknown command') {
    $proc.Kill()
    throw "Expected unknown-command error, got: $reply"
}

if ($Fonix -and $Wav) {
    if (-not (Test-Path $Fonix)) { throw "Fonix not found: $Fonix" }
    if (-not (Test-Path $Wav)) { throw "Wav not found: $Wav" }
    $bytes = [System.Text.Encoding]::UTF8.GetByteCount($Text)
    $proc.StandardInput.WriteLine('LIP')
    $proc.StandardInput.WriteLine('USEnglish')
    $proc.StandardInput.WriteLine((Resolve-Path $Fonix).Path)
    $proc.StandardInput.WriteLine((Resolve-Path $Wav).Path)
    $proc.StandardInput.WriteLine($lip)
    $proc.StandardInput.WriteLine("$bytes")
    $proc.StandardInput.Write($Text)
    $proc.StandardInput.WriteLine()
    $lipDeadline = [datetime]::UtcNow.AddMinutes(2)
    $lipReply = $null
    while ([datetime]::UtcNow -lt $lipDeadline) {
        $lipReply = $proc.StandardOutput.ReadLine()
        if ($null -eq $lipReply) { break }
        if ($lipReply.Trim().Length -eq 0) { continue }
        Write-Host $lipReply
        if ($lipReply.StartsWith('FXW ')) { break }
    }
    if ($lipReply -ne 'FXW OK' -or -not (Test-Path $lip) -or ((Get-Item $lip).Length -le 0)) {
        $proc.Kill()
        throw "LIP job failed: $lipReply"
    }
    Write-Host "LIP ok ($((Get-Item $lip).Length) bytes)"
}

$proc.StandardInput.WriteLine('QUIT')
$bye = $null
$byeDeadline = [datetime]::UtcNow.AddSeconds(30)
while ([datetime]::UtcNow -lt $byeDeadline) {
    $bye = $proc.StandardOutput.ReadLine()
    if ($null -eq $bye) { break }
    if ($bye.Trim().Length -eq 0) { continue }
    Write-Host $bye
    if ($bye.StartsWith('FXW ')) { break }
}
$proc.WaitForExit(15000)
if ($bye -ne 'FXW BYE') { throw "Expected FXW BYE, got: $bye" }
Write-Host "serve protocol ok, exit=$($proc.ExitCode)"
