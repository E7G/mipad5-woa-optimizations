# SPDX-License-Identifier: MIT
# Extract only the pure guard function: never run the wrapper or open a registry.
$ErrorActionPreference='Stop'
$file=Join-Path $PSScriptRoot 'Set-NabuGraphicsIdle.ps1'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($file,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Wrapper syntax invalid'}
$functions=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Get-DisplayOverrides'},$true))
if($functions.Count -ne 1){throw 'Unique guard function required'}
. ([scriptblock]::Create($functions[0].Extent.Text))
$script:Mock=@{}
function Read-Setting([string]$suffix){
    if($script:Mock.ContainsKey($suffix)){return @{Exists=$true;Kind='DWord';Value=$script:Mock[$suffix]}}
    return @{Exists=$false}
}
$cases=@('\DISPLAY','\DISPLAY\INTERNAL1','\DISPLAY\EXTERNAL3','\DISPLAY\EXTERNAL4',
         '\DISPLAY\WIRELESSDISPLAY3','\INTERNAL1','\EXTERNAL3')
if(@(Get-DisplayOverrides).Count -ne 0){throw 'Absent display settings must remain clear'}
foreach($path in $cases){
    $script:Mock=@{};$script:Mock[$path]=0
    $result=@(Get-DisplayOverrides)
    if($result.Count -ne 1 -or $result[0].Section -ne $path){throw ('Missed display setting: '+$path)}
}
$script:Mock=@{'\VIDEO'=0;'\ROTATOR'=0}
if(@(Get-DisplayOverrides).Count -ne 0){throw 'Other engines must not be classified as displays'}
Write-Output 'PASS: display namespace guard; all tests use mocked reads, no host registry access'
