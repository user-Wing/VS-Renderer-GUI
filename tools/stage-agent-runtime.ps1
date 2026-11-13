param(
    [string]$NodeDirectory = 'C:/Program Files/nodejs',
    [string]$SourceDirectory = (Join-Path $PSScriptRoot 'agent-runtime'),
    [string]$OutputDirectory = 'build/mingw-release/runtime/agent'
)
$ErrorActionPreference = 'Stop'
$nodeRoot = (Resolve-Path -LiteralPath $NodeDirectory).Path
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\','/')
$parent = Split-Path -Parent $output
if ((Split-Path -Leaf $output) -ne 'agent' -or !$parent -or $output -eq [IO.Path]::GetPathRoot($output)) { throw 'OutputDirectory must name an agent runtime directory' }
if ((Test-Path -LiteralPath $output) -and ((Get-Item -LiteralPath $output -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'OutputDirectory must not be a junction or symbolic link' }
$node = Join-Path $nodeRoot 'node.exe'
$npm = Join-Path $nodeRoot 'node_modules/npm'
$npmCli = Join-Path $npm 'bin/npm-cli.js'
foreach ($path in @($node,$npmCli)) { if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing Node/npm component: $path" } }
$nodeInfo = (& $node -p 'JSON.stringify({version:process.version,arch:process.arch})') | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $nodeInfo.version -notmatch '^v\d+\.\d+\.\d+$' -or [int]($nodeInfo.version.TrimStart('v').Split('.')[0]) -lt 20) { throw 'Node.js 20 or later is required' }
New-Item -ItemType Directory -Path $parent -Force | Out-Null
$work = Join-Path $parent ('.agent-stage-'+[guid]::NewGuid().ToString('N'))
$backup = Join-Path $parent ('.agent-previous-'+[guid]::NewGuid().ToString('N'))
function Remove-StageDirectory([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if ((Split-Path -Parent $full) -ne $parent -or (Split-Path -Leaf $full) -notmatch '^\.agent-(stage|previous)-[a-f0-9]{32}$') { throw 'Invalid Agent staging cleanup path' }
    if (Test-Path -LiteralPath $full) { Remove-Item -LiteralPath $full -Recurse -Force }
}
function Get-RuntimeRelativePath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (!$full.StartsWith($work+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Component path escaped the Agent staging directory' }
    return $full.Substring($work.Length+1).Replace('\','/')
}
function Invoke-AgentDownload([string]$Uri,[string]$OutFile='') {
    for ($attempt=1;$attempt -le 3;$attempt++) {
        try {
            if ($OutFile) { Invoke-WebRequest -Uri $Uri -OutFile $OutFile -TimeoutSec 60; return }
            return Invoke-RestMethod -Uri $Uri -TimeoutSec 60
        } catch {
            if (($_.Exception.Response -and [int]$_.Exception.Response.StatusCode -eq 404) -or $attempt -eq 3) { throw }
            Start-Sleep -Seconds 1
        }
    }
}
New-Item -ItemType Directory -Path $work | Out-Null
try {
    foreach ($name in @('package.json','package-lock.json','bridge.mjs','install.mjs')) {
        Copy-Item -LiteralPath (Join-Path $source $name) -Destination $work
    }
    Copy-Item -LiteralPath $node -Destination (Join-Path $work 'node.exe')
    & $node $npmCli --prefix $work ci --omit=dev --ignore-scripts --no-fund --no-audit
    if ($LASTEXITCODE -ne 0) { throw 'Locked Agent npm installation failed' }
    Copy-Item -LiteralPath $npm -Destination (Join-Path $work 'npm') -Recurse
    $licenses = Join-Path $work 'licenses'
    New-Item -ItemType Directory -Path $licenses | Out-Null
    $nodeLicense = Join-Path $licenses 'node-LICENSE.txt'
    if (Test-Path -LiteralPath (Join-Path $nodeRoot 'LICENSE') -PathType Leaf) {
        Copy-Item -LiteralPath (Join-Path $nodeRoot 'LICENSE') -Destination $nodeLicense
    } else {
        Invoke-AgentDownload "https://raw.githubusercontent.com/nodejs/node/$($nodeInfo.version)/LICENSE" $nodeLicense
    }
    $licensePattern = '^(?i:licen[sc]e|copying|notice|copyright)(?:[._-].*)?$'
    $packageFiles = @((Get-Item -LiteralPath (Join-Path $work 'npm/package.json')))
    foreach ($tree in @('node_modules','npm/node_modules')) {
        $packageFiles += @(Get-ChildItem -LiteralPath (Join-Path $work $tree) -Filter package.json -File -Recurse | Where-Object { $_.FullName.Replace('\','/') -match '/node_modules/(?:@[^/]+/)?[^/]+/package.json$' })
    }
    $records = @()
    foreach ($file in ($packageFiles | Sort-Object FullName -Unique)) {
        $package = Get-Content -LiteralPath $file.FullName -Raw | ConvertFrom-Json
        $license = $package.license
        if (!$license -and $package.licenses) { $license = ($package.licenses | ForEach-Object type) -join ' OR ' }
        if ($license -and $license -isnot [string]) { $license = $license.type }
        if ($license -eq 'Apache 2.0') { $license = 'Apache-2.0' }
        $legal = @(Get-ChildItem -LiteralPath $file.DirectoryName -File | Where-Object Name -Match $licensePattern)
        $restoredSources = @()
        if (!$legal.Count) {
            # Restore omitted notices from the exact published version's source commit, never a moving branch.
            $metadata = Invoke-AgentDownload ('https://registry.npmjs.org/'+[Uri]::EscapeDataString($package.name)+'/'+$package.version)
            $repository = if ($metadata.repository -is [string]) { $metadata.repository } else { $metadata.repository.url }
            if ($repository -notmatch 'github\.com[/:]([^/]+/[^/#]+?)(?:\.git)?$') { throw "Dependency has no pinned GitHub source for legal files: $($package.name)" }
            $repo = $Matches[1]
            if ($metadata.gitHead -notmatch '^[a-f0-9]{40}$') { throw "Dependency has no pinned source commit for legal files: $($package.name)" }
            $legalEntries = @()
            $metadataOnly = @{
                '@npmcli/agent@4.0.2'='db7852869c74ceb8985d7c36b291a2e7a866741c'
                'spdx-exceptions@2.5.0'='3aa64bec339abc6a3eca00c3436aaa7e154b8799'
                'spdx-license-ids@3.0.23'='a2a975daf48bbc2afdc8e7c304405b44c656fd53'
            }
            $candidates = @('LICENSE','LICENSE.md','LICENSE.txt','COPYING','NOTICE','NOTICE.md')
            if ($metadataOnly["$($package.name)@$($package.version)"] -eq $metadata.gitHead) { $candidates=@() }
            foreach ($name in $candidates) {
                $uri = "https://raw.githubusercontent.com/$repo/$($metadata.gitHead)/$name"
                try {
                    Invoke-AgentDownload $uri (Join-Path $file.DirectoryName $name)
                    $restoredSources += $uri
                    $legalEntries += $name
                } catch {
                    if (!$_.Exception.Response -or [int]$_.Exception.Response.StatusCode -ne 404) { throw }
                    $partial=Join-Path $file.DirectoryName $name;if(Test-Path -LiteralPath $partial){Remove-Item -LiteralPath $partial -Force}
                }
            }
            if (!$legalEntries.Count) {
                if ($license -notmatch '^[A-Za-z0-9.-]+$') { throw "Dependency has no standalone legal file or simple SPDX identifier: $($package.name)" }
                $uri = "https://raw.githubusercontent.com/spdx/license-list-data/v3.27.0/json/details/$license.json"
                $reference = Invoke-AgentDownload $uri
                if ($reference.licenseId -ne $license -or !$reference.licenseText) { throw "Cannot obtain SPDX license text: $($package.name)" }
                $notice = @("Package: $($package.name)@$($package.version)","Declared license: $license",
                    ('Author metadata: '+($package.author | ConvertTo-Json -Compress -Depth 4)),
                    "Source: https://github.com/$repo/tree/$($metadata.gitHead)",
                    'The published package and pinned source contain no standalone LICENSE/NOTICE file.',
                    'Original package metadata is retained. The following is an unmodified standard license reference, not an invented package copyright notice.',
                    "License reference: $uri",'',$reference.licenseText)
                [IO.File]::WriteAllText((Join-Path $file.DirectoryName 'LICENSE-SPDX.txt'),($notice -join [Environment]::NewLine),[Text.UTF8Encoding]::new($false))
                $restoredSources += $uri
            }
            $legal = @(Get-ChildItem -LiteralPath $file.DirectoryName -File | Where-Object Name -Match $licensePattern)
        }
        $relative = Get-RuntimeRelativePath $file.DirectoryName
        $records += [ordered]@{
            name = $package.name
            version = $package.version
            license = $license
            path = $relative
            licenseFiles = @($legal | ForEach-Object { Get-RuntimeRelativePath $_.FullName })
            restoredLegalSources = $restoredSources
        }
    }
    $manifest = [ordered]@{
        node = [ordered]@{ version=$nodeInfo.version; architecture=$nodeInfo.arch; license='MIT with bundled third-party notices'; licenseFile='licenses/node-LICENSE.txt'; sha256=(Get-FileHash -LiteralPath (Join-Path $work 'node.exe') -Algorithm SHA256).Hash.ToLowerInvariant() }
        packageLockSha256 = (Get-FileHash -LiteralPath (Join-Path $work 'package-lock.json') -Algorithm SHA256).Hash.ToLowerInvariant()
        packages = @($records | Sort-Object name,version,path)
    }
    [IO.File]::WriteAllText((Join-Path $work 'components.json'),($manifest | ConvertTo-Json -Depth 10)+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
    $npmPackage = Get-Content -LiteralPath (Join-Path $work 'npm/package.json') -Raw | ConvertFrom-Json
    $client = Get-Content -LiteralPath (Join-Path $work 'node_modules/@modelcontextprotocol/client/package.json') -Raw | ConvertFrom-Json
    $summary = @('# Agent 运行组件','',('- Node.js '+$nodeInfo.version+' ('+$nodeInfo.arch+')：MIT；完整内嵌依赖声明见 `licenses/node-LICENSE.txt`。'),
        ('- npm '+$npmPackage.version+'：'+$npmPackage.license+'；见 `npm/LICENSE`。'),
        ('- @modelcontextprotocol/client '+$client.version+'：'+$client.license+'；见 `node_modules/@modelcontextprotocol/client/LICENSE`。'),
        '- npm 仅以 `ci --omit=dev --ignore-scripts` 安装；不包含开发测试服务器、fixture 或测试脚本。',
        ('- 全部 '+$records.Count+' 个 npm 包实例的版本、许可证和随附法律文件路径见 `components.json`。'),'')
    [IO.File]::WriteAllText((Join-Path $work 'COMPONENTS.md'),($summary -join [Environment]::NewLine),[Text.UTF8Encoding]::new($false))
    if (Test-Path -LiteralPath (Join-Path $work 'node_modules/@modelcontextprotocol/server')) { throw 'Development MCP server must not enter the runtime' }
    if (Test-Path -LiteralPath $output) { Move-Item -LiteralPath $output -Destination $backup }
    try { Move-Item -LiteralPath $work -Destination $output } catch { if (Test-Path -LiteralPath $backup) { Move-Item -LiteralPath $backup -Destination $output }; throw }
    Remove-StageDirectory $backup
    Write-Output "Agent runtime staged: $output"
    Write-Output "Node $($nodeInfo.version); npm $($npmPackage.version); MCP client $($client.version); $($records.Count) package instances with legal files"
} finally {
    Remove-StageDirectory $work
}
