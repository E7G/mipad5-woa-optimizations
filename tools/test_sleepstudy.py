"""Synthetic, private-data-free regression test for the XML summarizer."""

import tempfile
from pathlib import Path

from summarize_sleepstudy import summarize


def main():
    xml = '''<SleepStudy xmlns="http://schemas.microsoft.com/sleepstudy/2012">
      <ScenarioInstance Type="Sleep" Ac="1" Duration="600000000" />
      <ScenarioInstance Type="Sleep" Ac="0" LocalTimestamp="2026-10-28T12:00:00"
        Duration="1200000000" LowPowerStateTime="300000000"
        HwLowPowerStateTime="0" EntryRemainingCapacity="1000"
        ExitRemainingCapacity="990">
        <TopBlockers>
          <TopBlocker Name="GPU (\\_SB.GPU0)" ScenarioActiveTimePercent="90" />
          <TopBlocker Name="unrelated app" ScenarioActiveTimePercent="10" />
        </TopBlockers>
        <Devices><Device Name="GPU (\\_SB.GPU0)">
          <Component Index="0" ScenarioActiveTimePercent="95" />
        </Device></Devices>
      </ScenarioInstance>
    </SleepStudy>'''
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "synthetic.xml"
        path.write_text(xml, encoding="utf-8")
        sessions = summarize(path)
    assert len(sessions) == 1
    assert sessions[0]["duration_minutes"] == 2
    assert sessions[0]["drips_percent"] == 25
    assert sessions[0]["hardware_drips_percent"] == 0
    assert sessions[0]["gpu_mmu_blockers"][0]["active_percent"] == "90"
    assert sessions[0]["gpu_components"][0]["index"] == "0"
    print("sleepstudy synthetic test: PASS")


if __name__ == "__main__":
    main()
