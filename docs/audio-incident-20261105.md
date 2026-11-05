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

**Next gate:** confirm whether the distorted notification occurred at 14:27:52, then replay the assigned WAV at unchanged volume. If distortion recurs, perform a short, reversible speaker **Audio enhancements Off** A/B test with human listening, restore the prior setting afterwards, and check for a new AudioDG crash. [Microsoft's audio troubleshooting guide](https://support.microsoft.com/en-us/windows/hardware/audio/fix-distorted-or-crackling-audio-in-windows) lists enhancements as one possible source of distortion, not a diagnosis for this device. If the crash still reproduces, use a bounded diagnostic that catches the writer before the heap-corruption detector; do not leave page heap or verifier enabled for normal use.

No driver, audio setting, system volume, notification policy or boot configuration was changed in this triage.

## Controlled playback follow-up

- With the speaker's **Audio enhancements** switched off through Windows Settings, the user heard **no output** from the same test WAV. After restoring **Device Default Effects**, output remained silent until the Windows audio services were restarted. The user then heard the three repetitions normally. Thus disabling enhancements is not a usable fix on this configuration.
- After a later normal reboot, the same WAV was audible but distorted at a measured Windows master volume of **52%**. Restarting `AudioEndpointBuilder` and `Audiosrv` did not clear this distortion. No new AudioDG Application Error appeared; the earlier heap-corruption dump is not proven to explain this playback distortion.
- A 20% volume test was inaudible to the user. At **35%**, the user heard the same WAV normally on a repeat test. The master volume was left at 35% as a reversible symptom workaround; the previous 52% value was saved privately. This is one speaker/listening test, not proof that all media or long-term audio are fixed.

**Next diagnostic gate:** compare a sufficiently audible 35% and 52% level using multiple media and, if available, a separate output path. Avoid changing unsigned APO/codec gains without a verified baseline and rollback. Do not describe the volume workaround as an AudioDG heap-corruption fix.
