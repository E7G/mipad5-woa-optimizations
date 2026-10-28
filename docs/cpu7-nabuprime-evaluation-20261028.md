# CPU7: current PEP vs NabuPrimeCore v1.0.2.2 (2026-10-28)

The tested tablet already uses the [XiaoMi9 CPU7 PEP improvement](https://github.com/qaz6750/XiaoMi9-Drivers/commit/d63d516eee65083d6501f45683a6640a1f597cb8) with a separate local null-context guard. That PEP adds Prime/domain2 support and core-parking selection changes. The exact local PEP binary is **not** published in this repository. [NabuPrimeCore v1.0.2.2](https://github.com/inciAli/NabuPrimeCore/releases/tag/v1.0.2.2) instead runs a separate kernel governor that directly writes the CPU7 OSM frequency state.

One CPU7-pinned, identical integer workload (400 million dependent iterations, checksum verified), three runs per state:

| State | Seconds | Median |
|---|---|---:|
| Existing PEP, earlier baseline | 0.682 / 0.680 / 0.684 | 0.682 |
| NabuPrime running, `MaxLevel=20` | 0.683 / 0.684 / 0.691 | 0.684 |
| NabuPrime running, repeat | 0.683 / 0.679 / 0.679 | 0.679 |
| NabuPrime forced to `MaxLevel=0` (control) | 2.390 / 2.454 / 2.453 | 2.453 |
| Existing PEP after removal | 0.682 / 0.679 / 0.678 | 0.679 |
| Existing PEP rechecked after UsbNabu installation | 0.685 / 0.681 / 0.680 | 0.681 |

The low-level control confirms the test can distinguish 825.6 MHz from higher CPU7 performance. **No measurable full-load gain** was found on this already-patched system. The upstream project's improvement against an unpatched stock platform should not be applied to this machine.

Other [NabuPrimeCore source](https://github.com/inciAli/NabuPrimeCore/blob/main/drivers/nabu-prime/prime.cpp) features: 50 ms load sampling, a 21-level frequency table, two-sensor thermal limiting, display-off parking, a startup LUT check and an optional log. Those could alter idle power or thermal behavior, but neither benefit was measured on this machine. The driver has no battery-vs-AC or Windows power-plan policy; `MaxLevel` is fixed until service restart.

During a short `MaxLevel=0` trial, 22 of 469 logged samples observed a nonzero level before NabuPrime reasserted level 0. This is consistent with another controller concurrently writing CPU7 (the installed PEP is a plausible source), though the log alone does not uniquely identify the writer. Keeping two independent governors on the same frequency register is not justified by the measured performance. NabuPrime was removed; the existing PEP remains.

NabuPrime and UsbNabu share a test-signing certificate. The upstream NabuPrime `uninstall.ps1` leaves that certificate alone; any custom cleanup that removes it could affect UsbNabu. This repository does not redistribute either driver's package or signing material.
