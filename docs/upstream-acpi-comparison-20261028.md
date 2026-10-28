# Deployed nabu ACPI versus Project Aloha main (2026-10-28)

This is a **read-only**, block-level comparison. The upstream artifact is the [Project Aloha nabu `DSDT.aml` at `ecac086b`](https://github.com/Project-Aloha/mu_aloha_platforms/blob/ecac086b16e8f406e1629913e0f9ab9a7f6bb9d8/Platforms/SurfaceDuo1Pkg/Device/xiaomi-nabu/ACPI/DSDT.aml), Git blob `6bf39aa6699079217b6c211a4b4f2e8813afc23e`, SHA-256 `8B5E8B7530EA1B110D232A362F367C736A7D4FA51D1CE555440D9F0832E09B17`. The tablet's **live** Windows ACPI registry DSDT was re-hashed on 2026-10-28: 324,285 bytes, SHA-256 `6958CC6F88CF083B4BCD5A4F05CF32538BEA98E84E96BE4861629ED9F92560AE`, matching the local snapshot used below. The DSDTs were decompiled locally with ACPICA `iasl` 20260408; the private AML/DSL files are **not** redistributed.

| Block | Decompiled comparison | What it establishes |
|---|---|---|
| `PRTC` (`ACPI000E`) | Identical 53-line block | The upstream DSDT does not contain an obvious PRTC method change that would fix this tablet's HAL 20 event. It does not prove the bridge transaction is healthy. |
| PEP method `G0MD` | Identical 3,512-line block | Upstream has the same GPU component-0 display-rail transition. Merely replacing this DSDT cannot supply a missing `G0MD` edit. |
| `MMU0`, `MMU1` | Identical 83-line and 28-line blocks | The compared SMMU ACPI declarations do not explain the measured MMU0 runtime activity by themselves. Driver/client votes still require tracing. |
| `GPU0` | Different | The deployed `DISPLAYS` list declares 3 displays (`INTERNAL1`, `EXTERNAL3`, `EXTERNAL4`); upstream declares 7 (`INTERNAL1`, `EXTERNAL1`–`EXTERNAL6`). The `GPU0` block also contains different panel configuration buffers and additional display power-state packages. |

The deployed `GPU0` display numbering and the upstream numbering are **not interchangeable**. On this tablet, previously retained single-component settings include `EXTERNAL3`; upstream has a different display enumeration and resource payload. A wholesale upstream DSDT/UEFI swap would change more than the suspected idle behavior and needs a separate panel-compatibility, graphics, boot and rollback test. This comparison is **not** a recommendation to flash it.

Next diagnostic target remains the owning graphics driver's runtime component activity: obtain a fresh battery-only screen-off trace of GPU0 component callbacks, DxgKrnl F-state events and MMU0 activity. Use the [bounded ETW summarizer](../tools/summarize_graphics_etw.py) and verify window coverage and event loss before interpreting missing callbacks. No boot image, ACPI table, driver or power setting was modified by this comparison.
