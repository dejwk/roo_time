#!/usr/bin/env python3
"""Measure calendar resources with a local 32-bit ARM toolchain; no Bazel."""

import argparse
import json
from pathlib import Path
import subprocess


def run(args, **kwargs):
    return subprocess.check_output(args, text=True, **kwargs)


def measure_sizes(compiler, nm, root, output, calendar, flags):
    types = ["TimeZone", "FixedTimeZone", "RecurringTimeZone"]
    if calendar:
        types += ["CalendarBoundary", "CalendarInterval", "TimeRange", "OffsetSpan"]
    header = "calendar_interval" if calendar else "timezone"
    source = output / "sizes.cpp"
    source.write_text(
        f'#include "roo_time/{header}.h"\nextern "C" {{\n'
        + "".join(f"char measured_{t}[sizeof(roo_time::{t})];\n" for t in types)
        + "}\nstatic_assert(sizeof(void*) == 4, \"Requires a 32-bit ABI\");\n"
    )
    obj = output / "sizes.o"
    subprocess.run([compiler, *flags, "-I", str(root / "src"), "-c", str(source),
                    "-o", str(obj)], check=True)
    result = {}
    for line in run([nm, "-S", "--format=posix", str(obj)]).splitlines():
        parts = line.split()
        if parts[0].startswith("measured_"):
            result[parts[0][9:]] = int(parts[3], 16)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--baseline", default="0f235f6")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    compiler = str(args.compiler.resolve())
    prefix = compiler.removesuffix("g++")
    size = prefix + "size"
    nm = prefix + "nm"
    flags = ["-std=c++17", "-mcpu=cortex-m0plus", "-mthumb", "-Os", "-DNDEBUG",
             "-fno-exceptions", "-fno-rtti", "-ffunction-sections",
             "-fdata-sections", "-fstack-usage"]
    baseline = output / "baseline-src"
    baseline.mkdir(exist_ok=True)
    paths = run(["git", "ls-tree", "-r", "--name-only", args.baseline, "src"],
                cwd=root).splitlines()
    for name in paths:
        target = baseline / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(run(["git", "show", f"{args.baseline}:{name}"], cwd=root))
    units = ["duration.cpp", "civil_day.cpp", "internal/calendar.cpp",
             "wall_time.cpp", "timezone.cpp"]
    records = []
    for tree, source_root, calendar in [("baseline", baseline, False),
                                         ("current", root, False),
                                         ("calendar", root, True)]:
        for recurring in [False, True]:
            label = tree + ("-recurring" if recurring else "-fixed")
            dest = output / label
            dest.mkdir(exist_ok=True)
            definitions = []
            if calendar:
                definitions.append("-DROO_TIME_CALENDAR_ENUMERATION")
            if recurring:
                definitions.append("-DROO_TIME_CALENDAR_RECURRING")
            sources = [source_root / "src/roo_time" / unit for unit in units]
            if calendar:
                sources.append(root / "src/roo_time/calendar_interval.cpp")
            sources.append(root / "test/calendar_resource_probe.cpp")
            objects = []
            for source in sources:
                obj = dest / (source.stem + ".o")
                subprocess.run([compiler, *flags, *definitions, "-I",
                                str(source_root / "src"), "-c", str(source),
                                "-o", str(obj)], check=True)
                objects.append(str(obj))
            elf = dest / "probe.elf"
            subprocess.run([compiler, *flags, *objects, "--specs=nano.specs",
                            "--specs=nosys.specs", "-Wl,--gc-sections", "-o",
                            str(elf)], check=True)
            fields = run([size, str(elf)]).splitlines()[-1].split()
            record = {"consumer": label, "text": int(fields[0]),
                      "data": int(fields[1]), "bss": int(fields[2])}
            record["flash"] = record["text"] + record["data"]
            records.append(record)
    before = measure_sizes(compiler, nm, baseline, output, False, flags)
    after = measure_sizes(compiler, nm, root, output, True, flags)
    for name in before:
        assert before[name] == after[name], (name, before[name], after[name])
    frames = []
    for usage in (output / "calendar-recurring").glob("*.su"):
        if usage.stem != "calendar_interval":
            continue
        for line in usage.read_text().splitlines():
            name, amount, kind = line.rsplit("\t", 2)
            frames.append({"function": name.split(":", 3)[-1],
                           "bytes": int(amount), "kind": kind})
    own = [f["bytes"] for f in frames if "EnumerateCalendarBoundaries(" in f["function"]]
    assert len(own) == 1 and own[0] <= 256, own
    report = {"compiler": run([compiler, "--version"]).splitlines()[0],
              "flags": flags, "baseline": args.baseline,
              "baseline_sizes": before, "current_sizes": after,
              "frames": frames, "consumers": records}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# Calendar interval resource acceptance", "",
             f"Measured 2026-11-10 with `{report['compiler']}` on the 32-bit",
             "ARM Cortex-M0+ ABI. These are linked bare-metal probes, not a full",
             "firmware or hardware performance benchmark. No flash target was assumed.", "",
             "Compiler flags: `" + " ".join(flags) + "`.",
             "Linked with `--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections`",
             "without LTO, using the default newlib startup. Flash is text + data;",
             "BSS excludes automatic stack storage.", "",
             "## Object sizes", "", "| Type | Before | After |",
             "| --- | ---: | ---: |"]
    for name, amount in after.items():
        lines.append(f"| `{name}` | {before.get(name, '—')} | {amount} |")
    lines += ["", "No timezone instance storage was added; only vtable entries changed.",
              "", "## Linked consumers", "",
              "The same probe uses virtual forward lookup in every variant. Baseline",
              f"is `{args.baseline}`; current adds calendar queries; calendar also calls",
              "hourly enumeration across a fold. The fixed probe uses +05:45; the",
              "recurring probe uses the EU pattern with base +01:00.", "",
              "| Consumer | Flash bytes | Delta from baseline | BSS bytes |",
              "| --- | ---: | ---: | ---: |"]
    for record in records:
        base = next(r for r in records if r['consumer'] == 'baseline-' +
                    record['consumer'].split('-')[-1])
        delta = record['flash'] - base['flash']
        lines.append(f"| {record['consumer']} | {record['flash']} | {delta:+} | {record['bss']} |")
    lines += ["", "## Compiler stack frames", "",
              "`-fstack-usage` reports static frames after inlining, including saved",
              "registers and compiler spills. The 256-byte gate applies to the",
              "enumerator's own frame. These figures do not claim a complete call-tree",
              "bound: provider calls, non-inlined helpers, runtime division/calendar",
              "conversion and visitor frames require additional stack.", "",
              "| Function | Bytes |", "| --- | ---: |"]
    for frame in frames:
        lines.append(f"| `{frame['function']}` | {frame['bytes']} |")
    lines += ["", "The initial enumerator frame was 264 bytes. Decoding only the fields",
              "needed for reverse lookup removed a redundant `DateTime` aggregate.", "",
              "## Reproduce", "", "Run from the repository root:", "", "```sh",
              "python3 test/measure_calendar_resources.py \\",
              "  --compiler /path/to/arm-none-eabi-g++ \\",
              "  --output /home/dawidk/.cache/roo_time/calendar-resources", "```", "",
              "The script exports baseline sources, builds six probes serially,",
              "records raw compiler `.su` output and `report.json`, verifies a 32-bit",
              "ABI, unchanged timezone instance sizes, and the enumerator frame gate.",
              "It does not invoke Bazel or alter Bazel caches/resource limits.", "",
              "`//:calendar_no_allocation_test` links the library statically, wraps",
              "malloc/calloc/realloc and overrides",
              "new/new[] to trap any allocation while exercising all units with fixed,",
              "recurring and synthetic multi-transition zones. Provider conformance",
              "tests verify span/candidate calls never exceed the caller's budget.", ""]
    (output / "report.md").write_text("\n".join(lines))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
