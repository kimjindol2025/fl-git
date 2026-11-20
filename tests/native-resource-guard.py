#!/usr/bin/env python3
"""Controller-only checks for the native C supervisor; never launches DB/Chrome.

Receipts are private local evidence. A PASS requires exit 0, a parsed result,
event checksum, strict sequence, and an empty owned-child drain.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time


def require(test, label):
    if not test:
        raise AssertionError(label)


def bounded(command, seconds, *, receipt_prefix, **kwargs):
    child = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
    started = time.monotonic(); stdout = stderr = b""; outcome = "COMPLETE"
    try:
        try:
            stdout, stderr = child.communicate(timeout=seconds)
        except subprocess.TimeoutExpired as first:
            stdout, stderr = first.output or b"", first.stderr or b""
            # subprocess.run(timeout=...) escalates to SIGKILL internally. Only
            # TERM this exact Popen child, then preserve evidence and block.
            child.terminate(); outcome = "DEADLINE_EXCEEDED"
            try:
                stdout, stderr = child.communicate(timeout=3)
            except subprocess.TimeoutExpired as error:
                stdout, stderr = error.output or stdout, error.stderr or stderr
                outcome = "OWNED_TERMINATION_INCOMPLETE"
                raise RuntimeError(outcome) from error
            raise RuntimeError(outcome)
        return subprocess.CompletedProcess(command, child.returncode, stdout, stderr)
    finally:
        receipt_prefix.with_suffix(".stdout").write_bytes(stdout)
        receipt_prefix.with_suffix(".stderr").write_bytes(stderr)
        receipt_prefix.with_suffix(".receipt.json").write_text(json.dumps({
            "outcome": outcome, "exit": child.poll(), "seconds": time.monotonic()-started,
            "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
            "stderr_sha256": hashlib.sha256(stderr).hexdigest(), "sigkill": 0,
        }, indent=2)+"\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--guard", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    args.evidence.mkdir(mode=0o700)
    records = []
    sentinel = subprocess.Popen([str(args.fixture), "sentinel"], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    sentinel_start_ticks = int(Path("/proc", str(sentinel.pid), "stat").read_text().rsplit(")", 1)[1].split()[19])
    try:
        for name, mode in [("normal", "normal"), ("budget", "budget"),
                           ("inherited-sigchld-ignore", "normal")]:
            command = [str(args.guard), "--evidence", str(args.evidence / name),
                       "--interval-ms", "20", "--max-ms", "5000", "--grace-ms", "2000"]
            if mode == "budget":
                command += ["--rss-limit", str(16 * 1024 * 1024)]
            command += ["--", str(args.fixture), mode]
            # Deliberately inherited SIG_IGN must be normalized by the supervisor.
            def ignored_child():
                signal.signal(signal.SIGCHLD, signal.SIG_IGN)
            started = time.monotonic()
            result = bounded(command, 12, receipt_prefix=args.evidence / name,
                                    preexec_fn=ignored_child if name.startswith("inherited") else None)
            final = json.loads((args.evidence / name / "result.json").read_bytes())
            raw = (args.evidence / name / "events.jsonl").read_bytes()
            events = [json.loads(line) for line in raw.splitlines()]
            require(final["events_sha256"] == hashlib.sha256(raw).hexdigest(), name + ": checksum")
            require([e["sequence"] for e in events] == list(range(1, len(events)+1)), name + ": sequence")
            require(final["remaining_children"] == 0, name + ": drain")
            require(sentinel.poll() is None, name + ": foreign process unaffected")
            require(all(e.get("pid") != sentinel.pid for e in events), name + ": foreign excluded")
            if mode == "budget":
                require(result.returncode == 2 and final["status"] == "BLOCKED", "budget: fail closed")
                require(final["cause"] == "RSS_LIMIT_EXCEEDED", "budget: preserve first cause")
                require(final["peak_rss_bytes"] > 16 * 1024 * 1024, "budget: actual overshoot observed")
                require(any(e["event"] == "SIGTERM" and e["result"] == 0 for e in events), "budget: owned TERM")
            else:
                require(result.returncode == 0 and final["status"] == "PASS", name + ": pass")
                marker = next(x for x in result.stdout.decode().splitlines() if x.startswith("DETACHED_GRANDCHILD="))
                grandchild = int(marker.split("=")[1])
                measured = [e for e in events if e["event"] == "MEASURE" and e["pid"] == grandchild]
                require(len(measured) >= 2, name + ": detached measured twice")
                require(any(e["event"] == "REAP" and e["pid"] == grandchild for e in events), name + ": subreaper drain")
                require(not any(e["event"] == "SIGTERM" for e in events), name + ": normal exit")
            records.append({"name": name, "exit": result.returncode, "elapsed": time.monotonic()-started,
                            "result": final, "events": len(events)})
        # A non-fresh evidence directory must reject before target execution.
        refused = bounded([str(args.guard), "--evidence", str(args.evidence / "normal"),
                                  "--", str(args.fixture), "success"], 3, receipt_prefix=args.evidence / "refused")
        require(refused.returncode == 2, "non-fresh evidence: prelaunch refusal")
        require(sentinel.poll() is None, "final foreign sentinel alive")
        sentinel.stdin.write(b"q");sentinel.stdin.flush()
        require(sentinel.wait(timeout=3) == 0, "sentinel normal exit")
        summary = {"status": "PASS", "runs": records, "refuse_nonfresh_exit": refused.returncode,
                   "foreign_sentinel": "UNTOUCHED_NORMAL_EXIT", "foreign_sentinel_pid": sentinel.pid,
                   "foreign_sentinel_start_ticks": sentinel_start_ticks, "foreign_sentinel_exit": sentinel.returncode, "db_runs": 0, "browser_runs": 0,
                   "sigkill": 0, "physical_hard_limit": False}
        (args.evidence / "summary.json").write_text(json.dumps(summary, indent=2)+"\n")
        print("RESOURCE_GUARD_CHECKS=PASS normal/detached/budget/foreign/status/evidence")
    finally:
        if sentinel.poll() is None:
            sentinel.stdin.write(b"q");sentinel.stdin.flush();sentinel.wait(timeout=3)


if __name__ == "__main__":
    main()
