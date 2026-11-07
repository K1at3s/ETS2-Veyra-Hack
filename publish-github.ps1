# Run from PowerShell. Git handles authentication; no token is stored here.
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [string]$SourcePath = $PSScriptRoot,
    [string]$Repository = 'https://github.com/K1at3s/ETS2-Veyra-Hack.git',
    [string]$CommitMessage = 'Update Veyra 0.9 sources and bilingual setup guide',
    [string]$StagingParent = [System.IO.Path]::GetTempPath()
)

$ErrorActionPreference = 'Stop'
if (Test-Path variable:PSNativeCommandUseErrorActionPreference) {
    $PSNativeCommandUseErrorActionPreference = $false
}
$staging = $null

function Invoke-ProjectGit {
    param([Parameter(Mandatory)][string[]]$GitArguments)
    & git @GitArguments
    if ($LASTEXITCODE -ne 0) { throw "Git failed (exit $LASTEXITCODE). Publication stopped." }
}

try {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw 'Install Git for Windows, then open a new PowerShell window.'
    }
    if ($Repository -match '^https?://[^/]*@') {
        throw 'Do not put credentials in the URL. Use Git Credential Manager.'
    }
    if ([string]::IsNullOrWhiteSpace($CommitMessage)) { throw 'CommitMessage cannot be empty.' }
    $source = (Resolve-Path -LiteralPath $SourcePath).Path
    $topFiles = @('.gitignore','app.rc','CMakeLists.txt','build-msvc.cmd',
        'README.md','BUILDING.md','CHANGELOG.md','publish-github.ps1')
    $folders = @('src','tests','assets','tools','.github')
    $files = @()
    foreach ($name in $topFiles) {
        $item = Get-Item -LiteralPath (Join-Path $source $name)
        if ($item.PSIsContainer) { throw "Expected a file: $name" }
        $files += $item
    }
    foreach ($name in $folders) {
        $folder = Get-Item -LiteralPath (Join-Path $source $name)
        if (-not $folder.PSIsContainer) { throw "Expected a folder: $name" }
        $children = @(Get-ChildItem -LiteralPath $folder.FullName -Recurse -Force)
        foreach ($item in @($folder) + $children) {
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                throw "Links/reparse points are not published: $($item.FullName)"
            }
        }
        $files += @($children | Where-Object { -not $_.PSIsContainer })
    }
    foreach ($item in $files) {
        if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
            throw "Links/reparse points are not published: $($item.FullName)"
        }
        if ($item.Extension -match '^\.(exe|dll|obj|pdb|dmp|log|zip|pem|key)$' -or
            $item.Name -match '^\.env($|\.)|^settings\.ini$') {
            throw "Excluded build/private file found in source inputs: $($item.Name)"
        }
    }
    Write-Host "Source: $source"
    Write-Host "Destination: $Repository"
    Write-Host "Selected source files: $($files.Count)"
    if (-not $PSCmdlet.ShouldProcess($Repository, 'Commit and push the selected Veyra source files')) {
        return
    }

    # Use a new clone: never change an existing local checkout or its remotes.
    $parent = (Resolve-Path -LiteralPath $StagingParent).Path
    $staging = Join-Path $parent ('veyra-publish-' + [guid]::NewGuid().ToString('N'))
    Invoke-ProjectGit -GitArguments @('clone','--',$Repository,$staging)
    & git -C $staging rev-parse --verify HEAD *> $null
    if ($LASTEXITCODE -ne 0) {
        Invoke-ProjectGit -GitArguments @('-C',$staging,'symbolic-ref','HEAD','refs/heads/main')
    }
    $branch = (Invoke-ProjectGit -GitArguments @('-C',$staging,'symbolic-ref','--short','HEAD') | Out-String).Trim()
    if ([string]::IsNullOrWhiteSpace($branch)) { throw 'Cannot determine publication branch.' }

    foreach ($item in $files) {
        $relative = $item.FullName.Substring($source.TrimEnd('\','/').Length).TrimStart('\','/')
        $destination = Join-Path $staging $relative
        $directory = Split-Path -Parent $destination
        $probe = $directory
        while ($probe -and $probe -ne $staging) {
            if (Test-Path -LiteralPath $probe) {
                $ancestor = Get-Item -LiteralPath $probe -Force
                if (-not $ancestor.PSIsContainer -or
                    ($ancestor.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
                    throw "Remote directory conflicts with source path: $relative"
                }
            }
            $probe = Split-Path -Parent $probe
        }
        if (-not (Test-Path -LiteralPath $directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        if (Test-Path -LiteralPath $destination) {
            $target = Get-Item -LiteralPath $destination -Force
            if ($target.PSIsContainer -or ($target.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
                throw "Remote path conflicts with source file: $relative"
            }
        }
        Copy-Item -LiteralPath $item.FullName -Destination $destination -Force
    }
    $addArguments = @('-C',$staging,'add','--') + $topFiles + $folders
    Invoke-ProjectGit -GitArguments $addArguments
    & git -C $staging diff --cached --quiet
    $difference = $LASTEXITCODE
    if ($difference -eq 0) {
        Write-Host 'Repository already contains these sources. Nothing to push.'
        Write-Host "Staging folder: $staging"
        return
    }
    if ($difference -ne 1) { throw 'Cannot inspect staged changes.' }
    & git -C $staging var GIT_AUTHOR_IDENT *> $null
    if ($LASTEXITCODE -ne 0) {
        throw 'Git author identity is missing. Set git config --global user.name and user.email, then rerun. No commit or push was made.'
    }
    Invoke-ProjectGit -GitArguments @('-C',$staging,'diff','--cached','--stat')
    Invoke-ProjectGit -GitArguments @('-C',$staging,'commit','-m',$CommitMessage)
    # Normal push only: remote changes are never discarded through --force.
    Invoke-ProjectGit -GitArguments @('-C',$staging,'push','origin',"HEAD:refs/heads/$branch")
    Write-Host "Published successfully to $Repository (branch $branch)."
    Write-Host "Staging clone retained at: $staging"
} catch {
    if ($staging) { Write-Warning "Staging clone retained for inspection: $staging" }
    throw
}
