<#
.SYNOPSIS
    Cube World Mod Loader Build Script (MSVC x64) - Modernized via CMake Presets
.DESCRIPTION
    Script de automação para compilação unificada com MSVC x64 de CubeForgeLoader (.dll / .fip) e testes.
    Utiliza por debaixo dos panos o novo sistema de CMakePresets.json do CMake 3.25+.
.EXAMPLE
    .\build.ps1
    .\build.ps1 -Target loader
    .\build.ps1 -Target test
    .\build.ps1 -InstallPath "C:\Program Files (x86)\Steam\steamapps\common\Cube World"
#>

[CmdletBinding()]
param (
    [ValidateSet("all", "loader", "test", "clean")]
    [string]$Target = "all",

    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string]$BuildType = "Release",

    [string]$InstallPath = ""
)

$ErrorActionPreference = "Stop"

$isWindows = $true
if ($PSVersionTable.PSVersion.Major -ge 6) {
    $isWindows = $IsWindows
}

# Map to the unified CMake Preset names based on Platform
$presetName = ""
$testPresetName = ""

if ($isWindows) {
    if ($env:VSCMD_ARG_TGT_ARCH -ne "x64" -or -not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $vcvars = $null
        if (Test-Path $vswhere) {
            $vsPath = & $vswhere -latest -prerelease -property installationPath
            if ($vsPath -and (Test-Path "$vsPath\VC\Auxiliary\Build\vcvars64.bat")) {
                $vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
            }
        }
        if (-not $vcvars) {
            $candidate = Get-ChildItem -Path "C:\Program Files\Microsoft Visual Studio", "C:\Program Files (x86)\Microsoft Visual Studio" -Filter "vcvars64.bat" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($candidate) { $vcvars = $candidate.FullName }
        }
        if ($vcvars) {
            Write-Host "Inicializando ambiente MSVC x64: $vcvars" -ForegroundColor Cyan
            cmd.exe /c "call `"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
                if ($_ -match "^(.*?)=(.*)$") {
                    [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
                }
            }
        }
    }
    $testPresetName = "windows-test"
    if ($BuildType -eq "Debug") {
        $presetName = "windows-debug"
    } else {
        $presetName = "windows-release"
    }
} else {
    $testPresetName = "macos-test"
    $presetName = "macos-debug" # Debug is used for macOS testing
    
    if ($Target -eq "loader") {
        Write-Error "O target 'loader' não é suportado no macOS/Linux. Apenas o target 'test' ou 'all' estão disponíveis para execução de testes unitários."
    }
    if ($InstallPath -ne "") {
        Write-Warning "Instalação desativada: O target de instalação só é suportado no Windows onde o CubeForgeLoader é compilado."
        $InstallPath = "" # Clear InstallPath to skip installation steps
    }
}

$buildDir = "build/$presetName"

if ($Target -eq "clean") {
    if (Test-Path "build") {
        Write-Host "Limpando diretórios de build..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build"
        Write-Host "Build limpo com sucesso." -ForegroundColor Green
    }
    exit 0
}

Write-Host "Configurando CMake usando o Preset: $presetName..." -ForegroundColor Cyan
cmake --preset $presetName

switch ($Target) {
    "all" {
        Write-Host "Compilando todo o projeto usando o Preset: $presetName..." -ForegroundColor Cyan
        cmake --build --preset $presetName --parallel
    }
    "loader" {
        Write-Host "Compilando target CubeForgeLoader..." -ForegroundColor Cyan
        cmake --build --preset $presetName --target CubeForgeLoader
    }
    "test" {
        Write-Host "Compilando e executando testes via CTest..." -ForegroundColor Cyan
        cmake --build --preset $presetName --target test_runner
        ctest --preset $testPresetName
    }
}

if ($InstallPath -ne "") {
    if (-not (Test-Path $InstallPath)) {
        Write-Error "Diretório de instalação não encontrado: $InstallPath"
    }

    Write-Host "Instalando binários em $InstallPath..." -ForegroundColor Magenta

    # Support both single-config generators (Ninja) and multi-config (Visual Studio) output structures
    $fipCandidates = @(
        (Join-Path $buildDir "src/CubeForgeLoader.fip"),
        (Join-Path $buildDir "src/$BuildType/CubeForgeLoader.fip")
    )
    $dllCandidates = @(
        (Join-Path $buildDir "src/CubeForgeLoader.dll"),
        (Join-Path $buildDir "src/$BuildType/CubeForgeLoader.dll")
    )

    $fipPath = $fipCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    $dllPath = $dllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($fipPath) {
        Copy-Item -Path $fipPath -Destination $InstallPath -Force
        Write-Host " -> Copiado CubeForgeLoader.fip ($fipPath)" -ForegroundColor Green
    } else {
        Write-Warning "CubeForgeLoader.fip não encontrado nos caminhos candidatos."
    }

    if ($dllPath) {
        Copy-Item -Path $dllPath -Destination $InstallPath -Force
        Write-Host " -> Copiado CubeForgeLoader.dll ($dllPath)" -ForegroundColor Green
    } else {
        Write-Warning "CubeForgeLoader.dll não encontrado nos caminhos candidatos."
    }
}

Write-Host "Processo concluído com sucesso!" -ForegroundColor Green
