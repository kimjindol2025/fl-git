#!/usr/bin/env python3
"""Bounded synthetic lifecycle regression checks; never runs a DB or browser.

Requires freshly built Guard, lifecycle C fixture, and the preserved fast-fork
fixture. Each scenario executes once. Any failure stops this batch; no retries.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time

# Reuse the existing controller's TERM-only bounded execution and checksum checks.
from importlib.util import module_from_spec, spec_from_file_location
spec = spec_from_file_location("existing_guard_checks", Path(__file__).with_name("native-resource-guard.py"))
existing = module_from_spec(spec)
spec.loader.exec_module(existing)


def inspect(directory):
    final = json.loads((directory / "result.json").read_bytes())
    raw = (directory / "events.jsonl").read_bytes()
    events = [json.loads(line) for line in raw.splitlines()]
    existing.require(final["events_sha256"] == hashlib.sha256(raw).hexdigest(), "checksum")
    existing.require([e["sequence"] for e in events] == list(range(1, len(events)+1)), "sequence")
    existing.require(final["remaining_children"] == 0 and final["final_drain_complete"], "final drain")
    existing.require(any(e["event"] == "FINAL_DRAIN" and e["complete"] for e in events), "drain evidence")
    existing.require(not any(e["event"] == "SIGTERM" for e in events), "normal termination only")
    existing.require(not final["physical_hard_limit"] and not final["independent_authority"], "scope")
    return final, events


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--guard", type=Path, required=True)
    p.add_argument("--lifecycle", type=Path, required=True)
    p.add_argument("--fast", type=Path, required=True)
    p.add_argument("--evidence", type=Path, required=True)
    args = p.parse_args()
    args.evidence.mkdir(mode=0o700)
    records = []
    for name, target, target_args in [("adopted", args.lifecycle, ["adopted"]),
                                       ("nonzero", args.lifecycle, ["nonzero"]),
                                       ("fast", args.fast, [])]:
        directory = args.evidence / name
        command = [str(args.guard), "--evidence", str(directory), "--interval-ms", "10",
                   "--max-ms", "3000", "--grace-ms", "1000", "--", str(target), *target_args]
        started = time.monotonic()
        result = existing.bounded(command, 8, receipt_prefix=args.evidence / name)
        final, events = inspect(directory)
        existing.require(result.returncode == (2 if name == "nonzero" else 0), name + ": exit")
        existing.require(final["status"] == ("BLOCKED" if name == "nonzero" else "PASS"), name + ": verdict")
        if name == "nonzero":
            existing.require(final["cause"] == "DRIVER_EXIT_NONZERO" and final["driver_exit"] == 7, "nonzero preserved")
        if name == "adopted":
            grandchild = int(next(x for x in result.stdout.decode().splitlines() if x.startswith("LIFECYCLE_ADOPTED=")).split("=")[1])
            owned = next(e for e in events if e["event"] == "OWNED" and e["pid"] == grandchild)
            root_reap = next(e for e in events if e["event"] == "REAP" and e["pid"] != grandchild)
            child_reap = next(e for e in events if e["event"] == "REAP" and e["pid"] == grandchild)
            existing.require(any(e["event"] == "MEASURE" and e["pid"] == grandchild for e in events), "adopted measured")
            existing.require(child_reap["sequence"] > root_reap["sequence"], "adopted child outlived driver")
            existing.require(owned["reason"] in ["child_or_subreaped", "descendant"], "owned path")
        deferred = [e for e in events if e["event"] == "PARENT_EXIT_RESCAN_REQUIRED"]
        rescanned = [e for e in events if e["event"] == "OWNED_DESCENDANTS_RECHECKED" and e["complete"]]
        existing.require(len(deferred) == final["parent_exit_rescans"], "deferred count")
        existing.require(all(e["pin_errno"] == 3 and e["rss_bytes"] is None and e["final_drain_required"] for e in deferred), "exit is not zero RSS")
        if deferred:
            existing.require(rescanned and rescanned[-1]["sequence"] > deferred[-1]["sequence"], "rescan after deferred exit")
        if name == "fast":
            existing.require("FAST_FORK_CHILDREN_NORMAL=256/256" in result.stdout.decode(), "fast actual children")
            existing.require(not any(e["event"] == "UNPINNED_ABSENCE_REJECTED" for e in events), "no ambiguous pin failure")
        records.append({"case": name, "exit": result.returncode, "status": final["status"],
                        "seconds": time.monotonic()-started, "events": len(events),
                        "parent_exit_rescans": len(deferred), "final": final})
        (args.evidence / "progress.json").write_text(json.dumps(records, indent=2)+"\n")
    summary = {"status": "PASS", "scenarios": len(records), "records": records,
               "prior_failures_reset": False, "db_runs": 0, "browser_runs": 0, "sigkill": 0}
    (args.evidence / "summary.json").write_text(json.dumps(summary, indent=2)+"\n")
    print("GUARD_LIFECYCLE_SCENARIOS=3/3")


if __name__ == "__main__":
    main()
