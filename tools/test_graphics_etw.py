"""Synthetic regression tests for bounded graphics ETW XML summarization."""

import json
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from summarize_graphics_etw import DXGK, NS, POWER, TRACE, summarize


def add_event(root, provider, event_id, stamp, **data):
    event = ET.SubElement(root, NS + "Event")
    system = ET.SubElement(event, NS + "System")
    ET.SubElement(system, NS + "Provider", Guid=provider)
    ET.SubElement(system, NS + "EventID").text = str(event_id)
    ET.SubElement(system, NS + "TimeCreated", SystemTime=stamp)
    payload = ET.SubElement(event, NS + "EventData")
    for key, value in data.items():
        ET.SubElement(payload, NS + "Data", Name=key).text = str(value)


def main():
    root = ET.Element("Events")
    add_event(root, TRACE, 0, "2026-10-28T09:59:00+08:00", EventsLost=0)
    add_event(root, POWER, 312, "2026-10-28T10:00:30+08:00",
              Token="0xSECRET", Component=0, Active="false")
    add_event(root, DXGK, 203, "2026-10-28T10:00:31+08:00",
              AdapterHandle="0xPRIVATE", ComponentIndex=6, State=1)
    add_event(root, DXGK, 204, "2026-10-28T10:00:32+08:00",
              AdapterHandle="0xPRIVATE", ComponentIndex=6)
    # A rundown after the window must still resolve earlier token callbacks.
    for device, token in (("GPU0", "0xSECRET"), ("MMU0", "0xM0"), ("MMU1", "0xM1")):
        add_event(root, POWER, 303, "2026-10-28T10:02:00+08:00",
                  Token=token, Id="\\_SB." + device)
    add_event(root, POWER, 312, "2026-10-28T10:03:00+08:00",
              Token="0xM0", Component=0, Active="true")
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "synthetic.xml"
        path.write_bytes(ET.tostring(root, encoding="utf-8"))
        result = summarize(path, "2026-10-28T10:00:00", "2026-10-28T10:01:00")
        assert result["window_covered"]
        assert result["max_events_lost"] == 0
        assert result["absence_interpretable"]
        assert result["power_callbacks"] == [
            {"device": "GPU0", "component": "0", "active": "false", "count": 1}]
        assert result["dxgk_fstate_starts_unattributed"] == [
            {"component": "6", "state": "1", "count": 1}]
        assert result["dxgk_fstate_ends_unattributed"] == [
            {"component": "6", "count": 1}]
        assert "SECRET" not in json.dumps(result)
        assert "PRIVATE" not in json.dumps(result)
        assert not summarize(path, "2026-10-28T09:00:00", "2026-10-28T10:01:00")[
            "absence_interpretable"]
        lossy = ET.fromstring(path.read_bytes())
        loss_field = lossy.find(".//" + NS + "Data[@Name='EventsLost']")
        loss_field.text = "2"
        path.write_bytes(ET.tostring(lossy, encoding="utf-8"))
        assert summarize(path, "2026-10-28T10:00:00", "2026-10-28T10:01:00")[
            "absence_interpretable"] is False
    print("graphics ETW synthetic test: PASS")


if __name__ == "__main__":
    main()
