# Notification distortion / AudioDG crash triage (2026-11-05)

The user reported an entire Windows Security notification sound as distorted. The exact notification timestamp has **not yet been confirmed** against the crash time. This note contains only sanitized read-only findings; the memory dumps and event logs remain private.

| Evidence | Result |
|---|---|
| Live Windows at 14:30 local | `Audiosrv` and `AudioEndpointBuilder` running; enumerated media/audio-endpoint devices had PnP problem code 0. The Qualcomm audio idle settings still read `ConservationIdleTime=5`, `PerformanceIdleTime=5`, `IdlePowerState=1`. |
| Application Error at 14:27:52 local, 2m42s after boot | `AUDIODG.EXE` exited with `0xC0000374` (heap corruption); a new 83.8 MB user-mode dump was collected. |
| New dump, matched Microsoft symbols | The detector was `ntdll!RtlpHpLfhSubsegmentDelayFreeListBatch` while `MMDevAPI!DeserializePropVariant`/`CRegistryPropertyStore::GetValue` was called from `audiodg!FillAPOInitSystemEffectsStructure` → `CAPOWrapperSrv::InitializeAPORemote`. The dump had no page heap/Application Verifier, so it identifies **where corruption was detected**, not the earlier writer. `ProxyAPO.dll` was loaded but not on the faulting stack; this does **not** prove it faulty. |
| Older 2026-10-18 dump | Also `0xC0000374`, with the same AudioDG APO-initialization path and LFH delayed-free-list detector, although the allocation site within MMDevAPI differed. This supports a recurrent failure mode rather than an isolated bad notification WAV. |
| Notification sound file | The `Notification.Default` assignment points to `Windows Notify System Generic.wav`; its SHA-256 on the tablet matched the host's stock file (`2FE09E9AAA34FF473DC40FE7B62F277E395A787DA0E76E0D6F94D3BD31FC6540`). This does not prove the reported Security notification used that exact event. |
| Follow-up at 14:47 local | No second AudioDG Application Error since the 14:25 boot. Audio services remained running. The active speaker endpoint still registered the two `ProxyAPO` effect CLSIDs; registration alone does not establish which effect wrote the corrupted heap memory. |

The installed `ProxyAPO.dll` hash is unchanged from the prior audit. The [Project Aloha nabu audio repository at `5d8dff7`](https://github.com/Project-Aloha/windows_oem_xiaomi_nabu/blob/5d8dff779a0fcf7ee210b08c086fb7e5f8f890b4/Platform/Drivers/Audio/AudMiniport/ProxyAPO.dll) still has the same upstream blob as the October check; that repository provides no newer `ProxyAPO` blob to test. Do not blindly reinstall a matching binary or blame `ProxyAPO` merely because it is loaded.

**Initial follow-up plan (partly completed below):** confirm whether the distorted notification occurred at 14:27:52, then replay the assigned WAV at unchanged volume. If distortion recurs, perform a short, reversible speaker **Audio enhancements Off** A/B test with human listening, restore the prior setting afterwards, and check for a new AudioDG crash. [Microsoft's audio troubleshooting guide](https://support.microsoft.com/en-us/windows/hardware/audio/fix-distorted-or-crackling-audio-in-windows) lists enhancements as one possible source of distortion, not a diagnosis for this device. If the crash still reproduces, use a bounded diagnostic that catches the writer before the heap-corruption detector; do not leave page heap or verifier enabled for normal use.

No driver, audio setting, system volume, notification policy or boot configuration was changed in this triage.

## Controlled playback follow-up

- With the speaker's **Audio enhancements** switched off through Windows Settings, the user heard **no output** from the same test WAV. After restoring **Device Default Effects**, output remained silent until the Windows audio services were restarted. The user then heard the three repetitions normally. Thus disabling enhancements is not a usable fix on this configuration.
- After a later normal reboot, the same WAV was audible but distorted at a measured Windows master volume of **52%**. Restarting `AudioEndpointBuilder` and `Audiosrv` did not clear this distortion. No new AudioDG Application Error appeared; the earlier heap-corruption dump is not proven to explain this playback distortion.
- A 20% volume test was inaudible to the user. At **35%**, the user heard the same WAV normally on a repeat test. This was a symptom comparison, **not a fix**. The original **52%** master volume was subsequently restored and confirmed; the temporary volume workaround is no longer active.

## Channel, loopback and codec follow-up

- At the restored 52% volume, the user heard obvious distortion in the full notification and in the right-only version of that same stock WAV. The left-only version sounded normal. After rotating the tablet and Windows display by 180 degrees, the left-only version remained normal and the right-only version remained distorted. The user could not confirm whether the physical speaker group emitting the right channel changed, so the rotation result does **not** isolate a particular speaker.
- A 52% Windows WASAPI loopback capture made while the user heard right-channel distortion had no digital clipping (peak absolute sample approximately 0.145). The captured right channel matched the original right-only source with explained variance approximately 0.99445 after alignment. This supports a fault **after the captured Windows render stream**; it does not precisely locate the fault within APO, codec, DSP, amplifier or speaker hardware. The original WAV itself is not an adequate explanation.
- The user played the same right-only WAV through Android on the same tablet at about half Android volume and heard normal output. Android and Windows volume scales are not directly comparable, but this argues against an obvious permanently damaged speaker.
- The live ACPI codec `_UID` values 0–3 map to I2C addresses `0x40`–`0x43`, and the installed Windows BR/TR/BL/TL profile mapping agrees with the Android DTS mapping. The four installed music-profile hashes matched the current [Project Aloha nabu driver repository](https://github.com/Project-Aloha/windows_oem_xiaomi_nabu). No obvious wrong-UID/wrong-profile mapping was found.
- The installed codec driver is version `11.28.24.473`, matching [remtrik/cs35l41_win](https://github.com/remtrik/cs35l41_win). Its codec start sequence resets the part, configures I2S, initializes and starts DSP, then enables the amplifier. A stale initialization/clock-sequencing state is a **hypothesis**, not a demonstrated source-code bug.

## Targeted reinitialization outcome

`Disable-PnpDevice` on `ACPI\\CSCL3541\\0` returned `NotImplemented` and did not isolate the amp. A subsequent `pnputil /restart-device ACPI\\CSCL3541\\0` returned success but left UID 0 at PnP code 22 (disabled). `pnputil /enable-device ACPI\\CSCL3541\\0` immediately restored code 0; all four codec devices were then code 0. This was a **runtime codec reinitialization**, not a driver replacement or permanent disable.

After UID 0 reinitialization, the user heard the right-only test, an 8-second stereo tone and a 12-second repeated original notification normally at the restored **52%** volume. The 12-second notification remained normal after one normal Windows reboot and a Windows → Android → Windows dual-boot cycle. This is an observed recovery of one episode, **not yet a proven reproducible or permanent fix**; the precise internal mechanism remains unproven. No codec binary, profile, Android boot image or persistent system-volume setting was changed.

**Next gate:** check the original 52% notification after a manual sleep/wake and again if distortion recurs during ordinary use. If it recurs, capture codec start/clock state before reinitialization, then compare with the recovered state. Do not call the temporary 35% volume comparison or the earlier AudioDG heap crash a proven root-cause fix.

## Manual standby attempt at 17:20

The user tried a roughly two-minute manual screen-off test after the reboot and dual-boot audio checks. The screen stayed black and a short power press did not restore it; the device later booted back to Windows. The user confirmed it **restarted on its own**, without a forced key-combination reboot. Windows logged Modern Standby entry at **17:20:26**, then Event 41 at **17:25:03** with `ConnectedStandbyInProgress=true`, `BugcheckCode=0`, and no intervening normal resume Event 507. No new minidump appeared. This is a **separate standby reliability failure**; it does not identify the audio-distortion cause or validate audio after a successful wake.

After the subsequent boot, all four CS35L41 devices again had PnP problem code 0. The speaker endpoint was active, unmuted, still at the original **52%** volume; the user heard another 12-second repeated stock notification **normally, without distortion** at 17:30. Do not repeat sleep testing until its black-screen risk is addressed.
