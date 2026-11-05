# NabuDisplayFix one-device trial (2026-11-05)

Upstream: [inciAli/NabuDisplayFix](https://github.com/inciAli/NabuDisplayFix), reviewed at commit `0f46c4e21705a63a3603230525ff2e938faa3991`. This is a **field result**, not a redistribution of Qualcomm's binary or an endorsement for all nabu panel variants.

## Compatibility and static audit

- The tablet's stock nabu C-PHY `qcdxkm8150.sys` was version `27.20.2140.0`, SHA-256 `8C62AFDE3009041BF05BCC5213582F3914C4A0AA3FAB6EC242CCE61665470B45`: exactly the upstream script's required input.
- Test signing was already enabled. The installed UsbNabu Type-C driver was `1.0.14.3`, satisfying upstream's `Live` prerequisite of 1.0.14 or newer.
- All six proposed file offsets contained the expected AArch64 NOP and were within `.text`. An offline reconstruction changed only the six instruction words, PE checksum/security-directory fields, and removed the original signature prior to re-signing. The on-tablet build reported a valid local test signature.
- The actual panel vendor was not established. Windows reported a generic integrated monitor identifier, which is not evidence of a CSOT or Tianma panel. Upstream reports testing only one CSOT device.

## Controlled result

1. The original QCDX service path and exact existing `ForceActive` values were backed up. A SYSTEM startup/time rollback was armed. The upstream `IdleOn`/`IdleOff` actions were **not** used because they change six sections at once, including previously retained individual settings.
2. `Live` hot-restart loaded the patched driver and PnP reported code 0, but the user saw **display corruption**. Hot-restarting the adapter with the stock service path restored the original loaded binary and normal display/touch. No new minidump was observed. This does not isolate whether the corruption came from the patch or from hot-restarting the display adapter.
3. A normal reboot loaded the patched binary with the previous power configuration; the user confirmed normal desktop and touch. The startup safeguard restored the **next boot path** to stock, leaving the patched binary in memory for this session only.
4. A second normal boot attempted only `DISPLAY\INTERNAL1 ForceActive=0` with the patched binary. Desktop and touch were normal. The startup safeguard removed that registry override shortly after boot, so this session **cannot prove** it remained effective through the later standby test.
5. On battery, a manual screen-off interval from 15:12:24 to 15:16:57 (about 4m34s) resumed to the original desktop with working display and touch. SleepStudy still recorded **0 software and hardware DRIPS transitions**. A single 135 mWh battery-gauge step is too coarse to establish a power-saving percentage.
6. At the user's choice, a final normal reboot loaded the **stock** driver. The stock service path, original power keys, PnP code 0, desktop and touch were verified. The temporary rollback task and local test-signing certificate were removed. Private backup and diagnostic files remain only on the test systems.

**Conclusion:** normal-boot loading and one short wake passed, while `Live` produced corruption. Neither a reliable power saving nor a fix for the full nabu standby blocker was demonstrated. Do not enable the upstream bulk `IdleOn` configuration based on this test. A future controlled trial would need an unambiguous panel identification, a boot-time single-component override that remains active for the measured interval, and bounded component-state tracing, with offline recovery prepared.

## Follow-up: retained inner-display override

A second normal-boot trial corrected the startup safeguard: its first run restored only the QCDX service path for the *next* boot and left `DISPLAY\INTERNAL1 ForceActive=0` in place for the measured session. Readback confirmed the patched binary was loaded, the inner-display value was still `0`, and GPU PnP code was 0. Existing `GRAPHICS`, `VIDEO`, `ROTATOR`, and `EXTERNAL3` single-component settings were unchanged.

With USB and charger disconnected, a bounded 16 MB ETW profile recorded the manual screen-off interval 16:06:57–16:12:27 (5m30s). The user resumed to the original desktop with normal display and touch. The trace covered the complete interval, reported zero lost events, and registered GPU0/MMU0/MMU1. GPU0 component 0 had **no idle callback** in the interval; component 1 did idle. SleepStudy reported **0 SW DRIPS, 0 HW DRIPS, and 0 DRIPS transitions**. The battery gauge did not step during this short sample, so it cannot quantify energy savings.

Therefore the startup-task issue was corrected, but the retained override plus patch **did not resolve the observed standby blocker**. The exact pretrial registry values were restored, recovery tasks removed, and a final normal reboot verified the stock QCDX binary loaded with PnP code 0 and working display/touch. Do not present this as a proven power-saving fix or leave the trial driver installed by default.
