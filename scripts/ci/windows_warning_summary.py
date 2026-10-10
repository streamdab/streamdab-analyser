#!/usr/bin/env python3
"""Summarise MSVC / linker / MSBuild warnings from a captured build log.

Usage: windows_warning_summary.py <build.log> [--max-annotations N]

Writes a Markdown report to $GITHUB_STEP_SUMMARY (when set) and to stdout, and
emits up to N `::warning` annotations for warnings in project code. Never fails
the job: it only reports. Warnings are de-duplicated (MSBuild repeats each one
once per project that includes the file).
"""
import argparse
import collections
import os
import re
import sys

# `path(line[,col]): warning C4244: message [project.vcxproj]`
#  `LINK : warning LNK4098: message`, `cl : Command line warning D9025: message`
WARNING_RE = re.compile(
    r"^\s*(?P<where>.*?)\s*:\s*(?:Command line )?warning\s+(?P<code>[A-Z]{1,4}\d{3,5})\s*:\s*(?P<msg>.*?)\s*$")
PROJECT_SUFFIX_RE = re.compile(r"\s*\[[^\]]*\.(?:vcxproj|proj)\]\s*$", re.IGNORECASE)
LOCATION_RE = re.compile(r"^(?P<file>.*?)\((?P<line>\d+)(?:,(?P<col>\d+))?\)$")
THIRD_PARTY_MARKERS = ("_deps", "vcpkg", "\\qt\\", "/qt/", "windows kits",
                       "microsoft visual studio", "program files")


def is_third_party(where: str) -> bool:
    low = where.lower()
    return any(marker in low for marker in THIRD_PARTY_MARKERS)


def parse(path: str):
    seen = set()
    found = []
    with open(path, encoding="utf-8", errors="replace") as handle:
        for raw in handle:
            match = WARNING_RE.match(raw.rstrip("\r\n"))
            if not match:
                continue
            msg = PROJECT_SUFFIX_RE.sub("", match["msg"])
            key = (match["where"], match["code"], msg)
            if key in seen:
                continue
            seen.add(key)
            found.append({"where": match["where"], "code": match["code"], "msg": msg,
                          "third_party": is_third_party(match["where"])})
    return found


def repo_relative(file: str) -> str:
    workspace = os.environ.get("GITHUB_WORKSPACE", "").replace("\\", "/").rstrip("/")
    norm = file.replace("\\", "/")
    if workspace and norm.lower().startswith(workspace.lower() + "/"):
        return norm[len(workspace) + 1:]
    return norm


def render(found):
    own = [w for w in found if not w["third_party"]]
    third = [w for w in found if w["third_party"]]
    lines = ["## Windows (MSVC) build warnings", "",
             f"Unique warnings: **{len(found)}** - project code: **{len(own)}**, "
             f"third-party / toolchain: **{len(third)}**", ""]
    if not found:
        lines.append("No compiler warnings in the build log.")
        return "\n".join(lines) + "\n", own
    for title, items in (("Project code", own), ("Third-party / toolchain", third)):
        if not items:
            continue
        by_code = collections.defaultdict(list)
        for item in items:
            by_code[item["code"]].append(item)
        lines += [f"### {title}", "", "| Code | Count | Example |", "|---|---:|---|"]
        for code, group in sorted(by_code.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            example = group[0]["msg"].replace("|", "\\|")[:110]
            lines.append(f"| {code} | {len(group)} | {example} |")
        lines.append("")
    if own:
        lines += ["<details><summary>Project warnings (first 200)</summary>", ""]
        for item in own[:200]:
            lines.append(f"- `{item['where']}` **{item['code']}** {item['msg']}")
        lines += ["", "</details>", ""]
    return "\n".join(lines) + "\n", own


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log")
    parser.add_argument("--max-annotations", type=int, default=10)
    args = parser.parse_args()
    if not os.path.exists(args.log):
        print(f"build log not found: {args.log}")
        return 0
    report, own = render(parse(args.log))
    sys.stdout.write(report)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as handle:
            handle.write(report)
    for item in own[:args.max_annotations]:
        loc = LOCATION_RE.match(item["where"])
        if loc:
            props = f"file={repo_relative(loc['file'])},line={loc['line']},title={item['code']}"
        else:
            props = f"title={item['code']}"
        print(f"::warning {props}::{item['msg']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
