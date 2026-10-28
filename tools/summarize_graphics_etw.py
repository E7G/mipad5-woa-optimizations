"""Stream a tracerpt XML export for nabu GPU/MMU runtime-power callbacks.

The output contains aggregate events only: no raw ETW XML, tokens, adapter
addresses, account names or machine identifiers. Timestamps use the local
clock written by tracerpt; the caller must supply start/end in that clock.
"""

import argparse
import collections
import datetime as dt
import json
import xml.etree.ElementTree as ET
from pathlib import Path

NS = "{http://schemas.microsoft.com/win/2004/08/events/event}"
POWER = "{331c3b3a-2005-44c2-ac5e-77220c37d6b4}"
DXGK = "{802ec45a-1e99-4b83-9920-87c98277ba9d}"
TRACE = "{9e814aad-3204-11d2-9a82-006008a86939}"
DEVICES = ("GPU0", "MMU0", "MMU1")


def local_time(value):
    """Do not normalize tracerpt's sometimes nonstandard timezone offset."""
    return dt.datetime.fromisoformat(value).replace(tzinfo=None)


def read_events(path):
    # Clearing the root as well as each Event keeps memory bounded; otherwise
    # ElementTree retains an empty child node for every exported event.
    context = ET.iterparse(path, events=("start", "end"))
    _, root = next(context)
    for kind, event in context:
        if kind != "end":
            continue
        if event.tag != NS + "Event":
            continue
        system = event.find(NS + "System")
        if system is None:
            root.clear()
            continue
        provider_node = system.find(NS + "Provider")
        stamp_node = system.find(NS + "TimeCreated")
        if provider_node is None or stamp_node is None:
            root.clear()
            continue
        guid = provider_node.get("Guid", "").lower()
        event_id = int(system.findtext(NS + "EventID", "-1"))
        stamp = local_time(stamp_node.get("SystemTime"))
        data = {node.get("Name"): "".join(node.itertext()).strip()
                for node in event.findall(NS + "EventData/" + NS + "Data")}
        yield guid, event_id, stamp, data
        root.clear()


def summarize(path, start, end):
    start, end = local_time(start), local_time(end)
    if start >= end:
        raise ValueError("start must precede end")

    tokens = {}
    first = last = None
    events_lost = []
    for guid, event_id, stamp, data in read_events(path):
        first = stamp if first is None else min(first, stamp)
        last = stamp if last is None else max(last, stamp)
        if guid == POWER and event_id in (303, 304):
            token, name = data.get("Token"), data.get("Id")
            if token and name:
                tokens[token] = name
        if guid == TRACE and event_id == 0 and "EventsLost" in data:
            events_lost.append(int(data["EventsLost"], 0))

    callbacks = collections.Counter()
    fstate_starts = collections.Counter()
    fstate_ends = collections.Counter()
    provider_events = collections.Counter()
    unregistered = 0
    for guid, event_id, stamp, data in read_events(path):
        if not start <= stamp < end:
            continue
        if guid in (POWER, DXGK):
            provider_events["Kernel-Power" if guid == POWER else "DxgKrnl"] += 1
        if guid == POWER and event_id == 312:
            name = tokens.get(data.get("Token"))
            if name is None:
                unregistered += 1
                continue
            device = next((item for item in DEVICES if name.endswith("." + item)), None)
            if device:
                callbacks[(device, data.get("Component", "?"), data.get("Active", "?"))] += 1
        if guid == DXGK and event_id == 203:
            # The adapter handle is deliberately not exported. These counts
            # cannot be attributed to GPU0 without a separate adapter map.
            fstate_starts[(data.get("ComponentIndex", "?"), data.get("State", "?"))] += 1
        if guid == DXGK and event_id == 204:
            fstate_ends[data.get("ComponentIndex", "?")] += 1

    def rows(counter, fields):
        return [dict(zip(fields, key if isinstance(key, tuple) else (key,)), count=count)
                for key, count in sorted(counter.items())]

    registered = [device for device in DEVICES
                  if any(name.endswith("." + device) for name in tokens.values())]
    coverage = first is not None and first <= start and end <= last
    return {
        "window_start": start.isoformat(),
        "window_end": end.isoformat(),
        "trace_first": first.isoformat() if first else None,
        "trace_last": last.isoformat() if last else None,
        "window_covered": coverage,
        "max_events_lost": max(events_lost) if events_lost else None,
        "registered_targets": registered,
        "provider_events_in_window": dict(provider_events),
        "unregistered_power_callbacks": unregistered,
        "power_callbacks": rows(callbacks, ("device", "component", "active")),
        "dxgk_fstate_starts_unattributed": rows(fstate_starts, ("component", "state")),
        "dxgk_fstate_ends_unattributed": rows(fstate_ends, ("component",)),
        "absence_interpretable": bool(coverage and events_lost and max(events_lost) == 0
                                      and set(DEVICES) <= set(registered)
                                      and provider_events["Kernel-Power"] > 0
                                      and provider_events["DxgKrnl"] > 0),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", type=Path, help="local tracerpt XML file; do not publish it")
    parser.add_argument("--start", required=True, help="screen-off start in tracerpt local clock")
    parser.add_argument("--end", required=True, help="screen-on time in tracerpt local clock")
    args = parser.parse_args()
    print(json.dumps(summarize(args.xml, args.start, args.end), indent=2))


if __name__ == "__main__":
    main()
