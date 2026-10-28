# USB-C charging A/B on one Mi Pad 5 (2026-10-28)

This is **one-device field evidence**, not a generic nabu driver recommendation. The charger was a Xiaomi-branded 120 W USB-C charger supplied with a K70-family phone; its exact model and USB PD contract were **not** captured. The same charger and cable were used in Windows desktop with Wi-Fi on, no video/game load, at about 63–69% battery.

| Type-C stack | Battery-side net rate after settling | Observed capacity |
|---|---:|---|
| Existing Qualcomm/ported Mi 9 Type-C + PMIC drivers | about **+0.7 W** after boot with charger connected; earlier hot-plug period showed net discharge | 21,470 → 21,605 mWh, 63 → 64% over about 6 min |
| [UsbNabu v1.0.14.19-r3](https://github.com/inciAli/UsbNabu/releases/tag/v1.0.14.19-r3) | about **+16–17.5 W** after the first minute | 21,336 → 23,318 mWh, 63 → 69% over about 6 min |

Both readings came from Windows `IOCTL_BATTERY_QUERY_STATUS` (`BATTERY_STATUS.Rate`), which reports **net battery charge/discharge power**, not USB-C input power. A positive rate means net charging; it does **not** reveal the negotiated PDO/PPS, cable current or charger output wattage. Percentage and mWh are gauge readings with quantization. No claim of reaching the tablet's advertised 33 W is made. See [Microsoft's structure definition](https://learn.microsoft.com/en-us/windows/win32/power/battery-status-str).

The candidate's `NabuTypeC` and `NabuSrcGuard` services were running, four checked Type-C/PMIC/USB devices had PnP problem code 0, and the battery later reached 72% with a +16.2 W reading. The user chose to retain UsbNabu. Original driver packages and registry bindings were backed up before installation; no Android boot, UEFI, PEP or display driver was changed. The trial timeout rollback was disabled after verification; a manual rollback copy remains local to the device.

**Not yet established:** charge-input watts, 33 W capability, long-term thermals/stability, charging after suspend, or charging and OTG simultaneously. A USB-C PD meter and controlled Android-vs-Windows comparison would be needed for those claims. The [nabu project status](https://github.com/erdilS/Port-Windows-11-Xiaomi-Pad-5/blob/main/guide/English/status.md) still describes general Windows charging support as slow; this particular A/B does not supersede its support matrix.

No third-party binaries, certificates, driver packages, logs containing device identifiers, or credentials are redistributed here.
