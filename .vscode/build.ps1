# Universal C++ build for Windows.
[CmdletBinding()]
param([switch]$Run)
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$Cxx = if ($env:CXX) { $env:CXX } else { 'I:\MSYS2\ucrt64\bin\g++.exe' }
$Std = if ($env:STD) { $env:STD } else { 'c++23' }
$Out = if ($env:OUT) { $env:OUT } else { 'aether.exe' }

$skip = '\\(build|out|\.git|\.vscode)\\'

$Sources = Get-ChildItem -Recurse -Filter *.cpp -File |
    Where-Object { $_.FullName -notmatch $skip } |
    ForEach-Object { $_.FullName }

if ($Sources.Count -eq 0) {
    Write-Error "No .cpp files found under $Root"
    exit 1
}

$Includes = Get-ChildItem -Recurse -Directory |
    Where-Object { $_.Name -in @('include','inc') -and $_.FullName -notmatch $skip } |
    ForEach-Object { "-I$($_.FullName)" }

Write-Host ">> $Cxx -std=$Std -g -Wall -Wextra $($Includes -join ' ') <$($Sources.Count) sources> -o $Out"
& $Cxx -std=$Std -g -Wall -Wextra @Includes @Sources -o $Out
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Run) { & ".\$Out" }