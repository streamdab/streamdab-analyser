#!/usr/bin/env python3
"""Check that an installed Windows package runs on its own.

Usage: verify_windows_deploy.py <install-prefix | extracted-zip-dir>

Runs the installed binaries with a PATH that contains ONLY the Windows system
directories - no Qt, no vcpkg, no Visual Studio - so a missing DLL (Qt, Qt-ADS,
FAAD2/FFTW/ZeroMQ, the C/C++ runtime) or a missing Qt plugin makes the check
fail instead of being masked by the developer/CI PATH.

  1. streamdab-cli.exe --version / --help       (Qt6Core + vcpkg DLLs)
  2. StreamDABAnalyser.exe --version            (Qt widgets, Qt-ADS, ... all load)
  3. StreamDABAnalyser.exe started as the GUI   (needs platforms/qwindows.dll);
     it must still be running after a few seconds.
"""
import os
import subprocess
import sys
import time

# NTSTATUS codes worth naming in the failure message.
KNOWN_EXIT_CODES = {
    0xC0000135: "STATUS_DLL_NOT_FOUND (a required DLL is missing)",
    0xC000007B: "STATUS_INVALID_IMAGE_FORMAT (32/64-bit or bad DLL mix)",
    0xC0000139: "STATUS_ENTRYPOINT_NOT_FOUND (wrong DLL version)",
    0xC0000005: "access violation",
}


def describe(code: int) -> str:
    unsigned = code & 0xFFFFFFFF
    return f"exit code {code} (0x{unsigned:08X}) {KNOWN_EXIT_CODES.get(unsigned, '')}".strip()


def clean_env() -> dict:
    root = os.environ.get("SystemRoot", r"C:\Windows")
    env = {
        "SystemRoot": root,
        "PATH": os.pathsep.join([os.path.join(root, "System32"), root]),
        "TEMP": os.environ.get("TEMP", ""),
        "TMP": os.environ.get("TMP", ""),
    }
    return env


def find_prefix(root: str) -> str:
    """`root` itself, or the directory below it that holds bin/StreamDABAnalyser.exe
    (an extracted CPack ZIP has one extra top-level folder)."""
    if os.path.exists(os.path.join(root, "bin", "StreamDABAnalyser.exe")):
        return root
    for dirpath, _dirs, files in os.walk(root):
        if "StreamDABAnalyser.exe" in files and os.path.basename(dirpath).lower() == "bin":
            return os.path.dirname(dirpath)
    return root


def run(cmd, env, timeout=120):
    print(f"$ {' '.join(cmd)}", flush=True)
    proc = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=timeout)
    for stream in (proc.stdout, proc.stderr):
        if stream.strip():
            print(stream.strip()[:1500], flush=True)
    return proc.returncode


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    prefix = find_prefix(os.path.abspath(sys.argv[1]))
    bindir = os.path.join(prefix, "bin")
    gui = os.path.join(bindir, "StreamDABAnalyser.exe")
    cli = os.path.join(bindir, "streamdab-cli.exe")
    env = clean_env()
    failures = []

    dlls = sorted(f for f in os.listdir(bindir) if f.lower().endswith(".dll")) if os.path.isdir(bindir) else []
    print(f"installed bin: {len(dlls)} top-level DLLs; "
          f"platforms plugin present: {os.path.exists(os.path.join(bindir, 'platforms', 'qwindows.dll'))}")
    print("PATH used for the checks: " + env["PATH"], flush=True)

    for exe in (cli, gui):
        if not os.path.exists(exe):
            failures.append(f"missing {exe}")

    if not failures:
        for exe, args in ((cli, ["--version"]), (cli, ["--help"]), (gui, ["--version"])):
            code = run([exe] + args, env)
            if code != 0:
                failures.append(f"{os.path.basename(exe)} {' '.join(args)}: {describe(code)}")

    if not failures:
        print("$ StreamDABAnalyser.exe (GUI start-up check)", flush=True)
        proc = subprocess.Popen([gui], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        deadline = time.time() + 10
        while time.time() < deadline and proc.poll() is None:
            time.sleep(0.5)
        if proc.poll() is not None:
            out, err = proc.communicate()
            print((out + err).strip()[:1500])
            failures.append(f"GUI exited during start-up: {describe(proc.returncode)}")
        else:
            print("GUI is running after 10 s - start-up OK", flush=True)
            proc.terminate()
            try:
                proc.wait(timeout=15)
            except subprocess.TimeoutExpired:
                proc.kill()

    if failures:
        print("\nWINDOWS DEPLOYMENT CHECK FAILED:")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("\nWindows deployment check passed: the installed package runs without Qt/vcpkg on PATH.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
