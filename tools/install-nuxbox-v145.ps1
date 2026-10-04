$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = Join-Path $root 'build\setup\vs_buildtools.exe'
if ((Get-AuthenticodeSignature -LiteralPath $installer).Status -ne 'Valid') {
    throw 'Visual Studio installer signature is not valid.'
}
$arguments = '--passive --wait --norestart --installPath "C:\BuildTools\2026" --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.VC.CMake.Project --add Microsoft.VisualStudio.Component.Windows11SDK.26100'
$process = Start-Process -FilePath $installer -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -PassThru
$process.WaitForExit()
if ($process.ExitCode -notin 0, 3010) { throw "Visual Studio installation failed: $($process.ExitCode)" }
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
