"""Read-only SleepStudy XML summary for nabu power investigations.

The input may contain private activity data; this tool prints only session,
GPU/MMU blocker and GPU component fields. It never uploads the XML.
"""

import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path


def local_name(tag):
    return tag.rsplit("}", 1)[-1]


def child(parent, name):
    return next((node for node in parent if local_name(node.tag) == name), None)


def parse_session(node):
    a = node.attrib
    blockers = child(node, "TopBlockers")
    devices = child(node, "Devices")
    selected = []
    if blockers is not None:
        for item in blockers:
            name = item.attrib.get("Name", "")
            if "GPU0" in name or "MMU0" in name or "MMU1" in name:
                selected.append({
                    "name": name,
                    "active_percent": item.attrib.get("ScenarioActiveTimePercent"),
                })
    gpu_components = []
    if devices is not None:
        for device in devices:
            if "GPU0" not in device.attrib.get("Name", ""):
                continue
            for component in device:
                if local_name(component.tag) == "Component":
                    gpu_components.append({
                        "index": component.attrib.get("Index"),
                        "active_percent": component.attrib.get("ScenarioActiveTimePercent"),
                    })
    return {
        "local_start": a.get("LocalTimestamp"),
        "duration_minutes": round(int(a.get("Duration", "0")) / 600_000_000, 2),
        "drips_percent": round(
            100 * int(a.get("LowPowerStateTime", "0")) / int(a["Duration"]), 2
        ) if int(a.get("Duration", "0")) else None,
        "hardware_drips_percent": round(
            100 * int(a.get("HwLowPowerStateTime", "0")) / int(a["Duration"]), 2
        ) if int(a.get("Duration", "0")) else None,
        "capacity_start_mwh": a.get("EntryRemainingCapacity"),
        "capacity_end_mwh": a.get("ExitRemainingCapacity"),
        "gpu_mmu_blockers": selected,
        "gpu_components": gpu_components,
    }


def summarize(path):
    root = ET.parse(path).getroot()
    return [parse_session(node) for node in root.iter()
            if local_name(node.tag) == "ScenarioInstance"
            and node.attrib.get("Type") == "Sleep"
            and node.attrib.get("Ac") == "0"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sleepstudy_xml", type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize(args.sleepstudy_xml), indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
