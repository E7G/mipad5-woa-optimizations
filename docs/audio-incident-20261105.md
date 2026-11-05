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
