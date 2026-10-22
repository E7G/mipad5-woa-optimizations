# SPDX-License-Identifier: MIT
# Public wrapper: no SSH, boot, driver installation, power action or automatic task.
[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='High')]
param(
    [ValidateSet('Inspect','Enable','Restore')][string]$Mode='Inspect',
    [string]$BackupPath,
    [switch]$AcknowledgeExperimental
)
$ErrorActionPreference='Stop'
$path='SOFTWARE\QCOM\Drivers\DxKMD'
$expectedGpu='8C62AFDE3009041BF05BCC5213582F3914C4A0AA3FAB6EC242CCE61665470B45'
if($env:OS -ne 'Windows_NT'){throw 'Windows required'}
$base=[Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine','Registry64')
function Read-Setting([string]$suffix) {
    $k=$base.OpenSubKey($path+$suffix)
    try {
        if(-not $k -or 'ForceActive' -notin $k.GetValueNames()){return @{Exists=$false}}
        return @{Exists=$true;Kind=$k.GetValueKind('ForceActive').ToString();Value=$k.GetValue('ForceActive')}
    } finally {if($k){$k.Dispose()}}
}
function Get-DisplayOverrides {
    # The tested binary builds DISPLAY\<enum>, not a bare INTERNAL1 section.
    # Also reject legacy/bare overrides rather than silently ignoring them.
    $names=@('INTERNAL1','INTERNAL2','INTERNAL3','EXTERNAL1','EXTERNAL2',
             'EXTERNAL3','EXTERNAL4','EXTERNAL5','EXTERNAL6',
             'WIRELESSDISPLAY1','WIRELESSDISPLAY2','WIRELESSDISPLAY3')
    $suffixes=@('\DISPLAY')
    foreach($name in $names){$suffixes+=('\DISPLAY\'+$name);$suffixes+=('\'+$name)}
    foreach($suffix in $suffixes){
        $setting=Read-Setting $suffix
        if($setting.Exists){[pscustomobject]@{Section=$suffix;Setting=$setting}}
    }
}
try {
    $current=Read-Setting '\GRAPHICS'
    if($Mode -eq 'Inspect') {
        [pscustomobject]@{Graphics=$current;Global=(Read-Setting '');
            PrimaryDisplay=(Read-Setting '\DISPLAY\INTERNAL1');
            LegacyBarePrimaryDisplay=(Read-Setting '\INTERNAL1');
            DisplayOverrides=@(Get-DisplayOverrides);Writes=$false}
        return
    }
    if(-not $AcknowledgeExperimental){throw 'Explicit -AcknowledgeExperimental required'}
    $identity=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if(-not $identity.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Administrator required'}
    if($env:PROCESSOR_ARCHITECTURE -ne 'ARM64'){throw 'Use native ARM64 PowerShell on the tablet'}
    $touch=$base.OpenSubKey('SYSTEM\CurrentControlSet\Enum\ACPI\NTTS3652')
    try {if(-not $touch){throw 'Expected nabu touch hardware not found'}} finally {if($touch){$touch.Dispose()}}
    if($Mode -eq 'Enable') {
        if((Read-Setting '').Exists -or @(Get-DisplayOverrides).Count -ne 0){throw 'Global/display override present: no write'}
        $service=$base.OpenSubKey('SYSTEM\CurrentControlSet\Services\QCDX')
        try {if(-not $service){throw 'QCDX service missing'};$image=[string]$service.GetValue('ImagePath')} finally {if($service){$service.Dispose()}}
        $image=[Environment]::ExpandEnvironmentVariables($image.Trim('"'))
        if($image.StartsWith('\SystemRoot\',[StringComparison]::OrdinalIgnoreCase)){$image=$env:windir+$image.Substring(11)}
        if($image.StartsWith('\??\')){$image=$image.Substring(4)}
        if(-not [IO.Path]::IsPathRooted($image) -or -not (Test-Path -LiteralPath $image -PathType Leaf)){throw 'GPU image path cannot be verified'}
        if((Get-FileHash -LiteralPath $image -Algorithm SHA256).Hash -ne $expectedGpu){throw 'GPU binary differs from tested version; no write'}
        if($current.Exists) {
            if($current.Kind -eq 'DWord' -and $current.Value -eq 0){Write-Output 'Already enabled; no write and no new rollback ownership';return}
            throw 'Existing engine value is not owned by this operation; no overwrite'
        }
        $key=$base.OpenSubKey($path+'\GRAPHICS');$keyExisted=($null -ne $key);if($key){$key.Dispose()}
        if(-not $PSCmdlet.ShouldProcess('GRAPHICS/ForceActive','Back up original absence and write DWORD 0')){return}
        if(-not $BackupPath){$BackupPath=Join-Path $env:ProgramData ('NabuGraphicsIdle/'+[Guid]::NewGuid().ToString()+'/before.json')}
        $BackupPath=[IO.Path]::GetFullPath($BackupPath)
        if(Test-Path -LiteralPath $BackupPath){throw 'Backup path exists; never overwrite'}
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($BackupPath))
        $backup=[ordered]@{Schema=1;Purpose='NabuGraphicsIdle';Path=$path+'\GRAPHICS';Name='ForceActive';OriginalValueAbsent=$true;OriginalKeyPresent=$keyExisted;Time=[DateTimeOffset]::Now.ToString('o')}
        # CreateNew prevents silently overwriting a raced/pre-existing baseline file.
        $bytes=[Text.UTF8Encoding]::new($false).GetBytes(($backup|ConvertTo-Json))
        $file=[IO.File]::Open($BackupPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
        try {$file.Write($bytes,0,$bytes.Length);$file.Flush($true)} finally {$file.Dispose()}
        if((Read-Setting '\GRAPHICS').Exists){throw 'Value appeared after baseline; no write'}
        $key=$base.CreateSubKey($path+'\GRAPHICS')
        try {$key.SetValue('ForceActive',0,[Microsoft.Win32.RegistryValueKind]::DWord);$key.Flush()} finally {$key.Dispose()}
        $after=Read-Setting '\GRAPHICS'
        if(-not $after.Exists -or $after.Kind -ne 'DWord' -or $after.Value -ne 0){throw 'Readback failed; inspect before retry'}
        [pscustomobject]@{Enabled=$true;BackupPath=$BackupPath;AutomaticRestart=$false}
    } else {
        if(-not $BackupPath){throw '-BackupPath required'}
        $item=Get-Item -LiteralPath $BackupPath
        if($item.Length -gt 16384){throw 'Backup outside size bound'}
        $backup=[IO.File]::ReadAllText($item.FullName)|ConvertFrom-Json
        if($backup.Schema -ne 1 -or $backup.Purpose -ne 'NabuGraphicsIdle' -or $backup.Path -ne ($path+'\GRAPHICS') -or $backup.Name -ne 'ForceActive' -or $backup.OriginalValueAbsent -ne $true -or $backup.OriginalKeyPresent -isnot [bool]){throw 'Invalid single-value baseline'}
        if(-not $current.Exists){Write-Output 'Already absent; no write';return}
        if($current.Kind -ne 'DWord' -or $current.Value -ne 0){throw 'Value changed since enable; no overwrite'}
        if(-not $PSCmdlet.ShouldProcess('GRAPHICS/ForceActive','Delete only the added DWORD 0')){return}
        $parent=$base.OpenSubKey($path,$true);$key=$null
        try {
            $key=$parent.OpenSubKey('GRAPHICS',$true)
            if(-not $key -or $key.GetValueKind('ForceActive') -ne [Microsoft.Win32.RegistryValueKind]::DWord -or $key.GetValue('ForceActive') -ne 0){throw 'State changed; stop restore'}
            $key.DeleteValue('ForceActive',$true);$key.Flush()
            if(-not $backup.OriginalKeyPresent -and $key.ValueCount -eq 0 -and $key.SubKeyCount -eq 0){$key.Dispose();$key=$null;$parent.DeleteSubKey('GRAPHICS',$false);$parent.Flush()}
        } finally {if($key){$key.Dispose()};if($parent){$parent.Dispose()}}
        if((Read-Setting '\GRAPHICS').Exists){throw 'Restore readback failed'}
        [pscustomobject]@{Restored=$true;AutomaticRestart=$false;RuntimeDriverMayStillCacheSetting=$true}
    }
} finally {$base.Dispose()}
