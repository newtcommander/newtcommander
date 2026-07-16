# gen_plugins_filter.ps1 - plugin build policy stage of build.cmd
#
# Reads the repository-root plugins.cfg, validates it against the plugin
# directories under src\plugins\, generates a solution filter
# (salamand.gen.slnf) that excludes disabled plugins from the MSBuild
# solution build, and (when -OutputPluginsDir is given) reconciles the
# build output with the policy: output directories of disabled or
# removed plugins are deleted and plugins.ver is filtered down to the
# enabled set.
#
# Exit code 0 = success; 1 = validation or I/O error (messages are
# printed as "ERROR: ..." lines). See
# specs/007-plugin-build-policy/contracts/build-cmd.md for the contract.

param(
    [Parameter(Mandatory = $true)][string]$Config,
    [Parameter(Mandatory = $true)][string]$Solution,
    [Parameter(Mandatory = $true)][string]$OutSlnf,
    [Parameter(Mandatory = $true)][string]$PluginsRoot,
    [string]$OutputPluginsDir
)

$ErrorActionPreference = 'Stop'
$errors = New-Object System.Collections.Generic.List[string]

# --- V1: config file must exist -------------------------------------------
if (-not (Test-Path -LiteralPath $Config -PathType Leaf)) {
    Write-Output ("ERROR: plugins.cfg not found at '{0}'. This file is the plugin" -f $Config)
    Write-Output '       build policy: one <plugin>=on|off line per plugin directory'
    Write-Output '       under src\plugins\. It must exist in the repository root.'
    exit 1
}

# --- discover plugin directories (the "shared" dir is build infrastructure)
$pluginDirs = @{}
foreach ($d in (Get-ChildItem -LiteralPath $PluginsRoot -Directory)) {
    if ($d.Name -ne 'shared') { $pluginDirs[$d.Name.ToLowerInvariant()] = $d.Name }
}

# --- parse + validate the config (V2 syntax, V3 unknown, V4 duplicate) ----
$entries = @{}   # lowercased name -> $true (on) / $false (off)
$lineNo = 0
foreach ($raw in (Get-Content -LiteralPath $Config)) {
    $lineNo++
    $line = $raw.Trim()
    if ($line -eq '' -or $line.StartsWith('#')) { continue }
    if ($line -match '^(?<name>[A-Za-z0-9_][A-Za-z0-9_-]*)\s*=\s*(?<state>on|off)$') {
        $name = $Matches['name'].ToLowerInvariant()
        $state = $Matches['state'].ToLowerInvariant()
        if ($entries.ContainsKey($name)) {
            $errors.Add(("ERROR: plugins.cfg line {0}: duplicate entry '{1}' - each plugin may be listed only once." -f $lineNo, $Matches['name']))
            continue
        }
        if (-not $pluginDirs.ContainsKey($name)) {
            $errors.Add(("ERROR: plugins.cfg line {0}: unknown plugin '{1}' - no directory src\plugins\{1} exists." -f $lineNo, $Matches['name']))
            continue
        }
        $entries[$name] = ($state -eq 'on')
    }
    else {
        $errors.Add(("ERROR: plugins.cfg line {0}: cannot parse '{1}' - expected <plugin>=on|off, '#' comment, or blank line." -f $lineNo, $raw.Trim()))
    }
}

# --- V5: every plugin directory must have an entry -------------------------
foreach ($key in ($pluginDirs.Keys | Sort-Object)) {
    if (-not $entries.ContainsKey($key)) {
        $errors.Add(("ERROR: plugins.cfg: plugin directory src\plugins\{0} has no entry - add '{0}=on' or '{0}=off'." -f $pluginDirs[$key]))
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Output $_ }
    exit 1
}

$enabled = @($entries.GetEnumerator() | Where-Object { $_.Value } | ForEach-Object { $_.Key })
$disabled = @($entries.GetEnumerator() | Where-Object { -not $_.Value } | ForEach-Object { $_.Key })

# --- generate the solution filter ------------------------------------------
# Include every project of the solution whose path is not under
# ..\plugins\<name>\ for a disabled plugin. Order follows the solution.
if (-not (Test-Path -LiteralPath $Solution -PathType Leaf)) {
    Write-Output ("ERROR: solution not found: {0}" -f $Solution)
    exit 1
}
$projects = New-Object System.Collections.Generic.List[string]
foreach ($line in [System.IO.File]::ReadAllLines($Solution)) {
    if ($line -match '^Project\("\{[^}]+\}"\) = "[^"]+", "([^"]+)", "\{[^}]+\}"') {
        $path = $Matches[1]
        if (-not $path.ToLowerInvariant().EndsWith('.vcxproj')) { continue }  # skip solution folders
        $skip = $false
        $lower = $path.ToLowerInvariant()
        foreach ($name in $disabled) {
            if ($lower.Contains("\plugins\$name\")) { $skip = $true; break }
        }
        if (-not $skip) { $projects.Add($path) }
    }
}
if ($projects.Count -eq 0) {
    Write-Output ("ERROR: no projects found in {0} - solution filter would be empty." -f $Solution)
    exit 1
}
$slnfProjects = ($projects | ForEach-Object { '      "' + ($_ -replace '\\', '\\') + '"' }) -join ",`r`n"
$slnfDir = Split-Path -Parent $OutSlnf
$slnRelative = Split-Path -Leaf $Solution   # slnf sits next to the solution
$slnf = "{`r`n  ""solution"": {`r`n    ""path"": ""$slnRelative"",`r`n    ""projects"": [`r`n$slnfProjects`r`n    ]`r`n  }`r`n}`r`n"
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutSlnf, $slnf, $utf8NoBom)

# --- reconcile the build output with the policy -----------------------------
if ($OutputPluginsDir -and (Test-Path -LiteralPath $OutputPluginsDir -PathType Container)) {
    foreach ($d in (Get-ChildItem -LiteralPath $OutputPluginsDir -Directory)) {
        $name = $d.Name.ToLowerInvariant()
        if ($name -eq 'intermediate') { continue }  # shared build intermediates, not a plugin
        $isEnabled = ($entries.ContainsKey($name) -and $entries[$name])
        if (-not $isEnabled) {
            # disabled plugin, or a leftover directory of a removed/unknown plugin
            Remove-Item -LiteralPath $d.FullName -Recurse -Force
            Write-Output ("Reconcile: removed stale output {0}" -f $d.FullName)
        }
    }
    $verFile = Join-Path $OutputPluginsDir 'plugins.ver'
    if (Test-Path -LiteralPath $verFile -PathType Leaf) {
        $verLines = [System.IO.File]::ReadAllLines($verFile)
        $kept = New-Object System.Collections.Generic.List[string]
        if ($verLines.Count -gt 0) { $kept.Add($verLines[0]) }  # version header
        for ($i = 1; $i -lt $verLines.Count; $i++) {
            $l = $verLines[$i]
            if ($l -match '^\d+:(?<first>[^\\/]+)[\\/]') {
                $first = $Matches['first'].ToLowerInvariant()
                if ($entries.ContainsKey($first) -and $entries[$first]) { $kept.Add($l) }
            }
            elseif ($l.Trim() -ne '') { $kept.Add($l) }
        }
        if ($kept.Count -ne $verLines.Count) {
            [System.IO.File]::WriteAllLines($verFile, $kept, [System.Text.Encoding]::ASCII)
            Write-Output ("Reconcile: plugins.ver filtered to {0} entries" -f ($kept.Count - 1))
        }
    }
}

Write-Output ("Plugins: {0} enabled, {1} disabled" -f $enabled.Count, $disabled.Count)
exit 0
