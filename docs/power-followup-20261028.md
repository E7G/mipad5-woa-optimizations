# Power follow-up: evidence before more changes (2026-10-28)

Current policy is balanced. On battery, processor minimum is **5%**, energy-performance preference **80**, maximum **100%**. This is a prior, reversible mild efficiency setting; no new CPU cap was applied in this follow-up. Plugged-in processor minimum remains 56%, and the sleep policy was left untouched.

A 20-second desktop process sample showed no substantial user-mode background load: `System` used about 0.5 CPU-second (2.5% of one core); the security service about 0.05 CPU-second. `powercfg /requests` showed no active application requests. Removing more services is unlikely to explain the remaining sleep blocker.

The last controlled, unplugged, **33-minute manual standby** (2026-10-23) resumed normally. SleepStudy reported 0% SW/HW DRIPS and roughly 97% active time for GPU0 and MMU0. Battery capacity fell one 135 mWh gauge step (58 → 57%), implying about 0.25 W or 0.73% of full capacity per hour **if the coarse gauge step is representative**. This is one session, not a validated before/after saving percentage. The observed blocker is below the app/service layer. Previous global GPU `ForceActive=0` caused a black-screen wake failure and was reverted; this repository does not recommend it.

For active use, the display was observed at **120 Hz and 84% brightness** while charging. These are plausible, user-visible power levers, not proof of their contribution. Do not silently change refresh or brightness: benchmark an opt-in battery-only display profile against the current state, preserving AC settings and an immediate rollback. Keep the already-selected sleep/network trade-off and CPU maximum performance unless the user explicitly changes them.

Near-term priorities:

1. Measure battery-only screen-on power at fixed brightness/refresh before and after any opt-in display change.
2. Repeat a clean 30–60 minute manual standby under similar battery/Wi-Fi/temperature conditions; compare mWh and SleepStudy, not only percentage.
3. Investigate GPU0/MMU0 component activity with read-only tracing before any further single-engine GPU/UEFI experiment. Do not alter CXPA2 or global display/GPU force-active settings based on this report.

No new driver, boot, BCD, ACPI or standby policy change was made by this audit.
