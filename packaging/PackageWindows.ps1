# 把 Release 构建打成 Windows 免安装 ZIP。
#
# 只在 GitHub Actions 的 windows-2022 runner 上运行：依赖已经激活的 MSVC 开发环境
# （VCToolsRedistDir）和 PATH 上的 Qt 工具（windeployqt、qmake）。
#
# 用法：PackageWindows.ps1 -BuildDir <构建目录> -ZipName <ZIP 文件名> -DisplayVersion <显示版本>
# 产出（当前目录）：<ZIP>、<ZIP>.sha256

param(
    [Parameter(Mandatory = $true)][string] $BuildDir,
    [Parameter(Mandatory = $true)][string] $ZipName,
    [Parameter(Mandatory = $true)][string] $DisplayVersion
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
$here = $PSScriptRoot
$package = Join-Path $PWD 'package'
$exe = Join-Path $package 'money-under-bed-deskpet.exe'

Write-Host '::group::Install and deploy the portable directory'
cmake --install $BuildDir --prefix $package

windeployqt --release --no-translations --no-compiler-runtime `
    --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw `
    --skip-plugin-types generic,iconengines,imageformats,networkinformation,tls `
    $exe

# windeployqt 只按桌面运行路径收集 qwindows；发行包自检还需要 offscreen，
# 必须放进包内，不能从 runner 的 Qt 目录借用。
$qtPlugins = (& qmake -query QT_INSTALL_PLUGINS).Trim()
$offscreen = Join-Path $qtPlugins 'platforms\qoffscreen.dll'
if (-not (Test-Path $offscreen)) { throw "Qt offscreen platform plugin not found: $offscreen" }
New-Item -ItemType Directory -Force (Join-Path $package 'platforms') | Out-Null
Copy-Item $offscreen (Join-Path $package 'platforms\qoffscreen.dll')

# 真正免安装：直接部署 app-local CRT DLL，不附带运行库安装器。
$crt = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir 'x64') `
    -Filter 'Microsoft.VC*.CRT' -Directory | Select-Object -First 1
if (-not $crt) { throw "MSVC runtime folder not found under $env:VCToolsRedistDir" }
Copy-Item (Join-Path $crt.FullName 'msvcp140*.dll') $package
Copy-Item (Join-Path $crt.FullName 'vcruntime140*.dll') $package
foreach ($required in 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll') {
    if (-not (Test-Path (Join-Path $package $required))) {
        throw "Missing app-local MSVC runtime: $required"
    }
}

@(
    "commit=$env:GITHUB_SHA"
    "version=$DisplayVersion"
    "qt=$env:QT_VERSION"
    "platform=windows-x86_64"
    "runner=windows-2022"
) | Set-Content (Join-Path $package 'BUILD_INFO.txt') -Encoding utf8
Write-Host '::endgroup::'

Write-Host '::group::Verify the package'
# GUI 子系统：正式包不弹控制台窗口。
if ((dumpbin /headers $exe | Out-String) -notmatch 'Windows GUI') {
    throw 'Packaged executable is not using the Windows GUI subsystem.'
}

cmake "-DPACKAGE_ROOT=$package" `
    "-DPACKAGE_EXECUTABLE=money-under-bed-deskpet.exe" `
    "-DPACKAGE_REQUIRED_FILES=platforms/qwindows.dll;platforms/qoffscreen.dll;licenses/msvc-runtime.md" `
    "-DPACKAGE_FORBIDDEN_PATHS=d3dcompiler_47.dll;dxcompiler.dll;dxil.dll;opengl32sw.dll;generic;iconengines;imageformats;networkinformation;tls;Qt6Svg.dll" `
    -P (Join-Path $here 'VerifyPackage.cmake')

# GUI 子系统程序在 shell 里直接调用不会等待结束，退出码必须用 Start-Process -Wait 取。
$env:QT_QPA_PLATFORM = 'offscreen'
$process = Start-Process -FilePath $exe -ArgumentList '--self-test' `
    -WorkingDirectory $package -Wait -PassThru -NoNewWindow
Remove-Item Env:QT_QPA_PLATFORM
Write-Host "packaged self-test exit=$($process.ExitCode)"
if ($process.ExitCode -ne 0) { throw "Packaged self-test failed with exit code $($process.ExitCode)" }

# 自检会写本地日志，再核对一次，确保没有日志之类的东西混进包里。
cmake "-DPACKAGE_ROOT=$package" `
    "-DPACKAGE_EXECUTABLE=money-under-bed-deskpet.exe" `
    -P (Join-Path $here 'VerifyPackage.cmake')
Write-Host '::endgroup::'

Compress-Archive -Path (Join-Path $package '*') -DestinationPath $ZipName
$hash = (Get-FileHash $ZipName -Algorithm SHA256).Hash.ToLower()
[IO.File]::WriteAllText((Join-Path $PWD "$ZipName.sha256"), "$hash  $ZipName`n", [Text.Encoding]::ASCII)
Write-Host "SHA-256: $hash"
