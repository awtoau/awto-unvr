#!/usr/bin/env python3
"""Open a terminal in the running VS Code window and run a command in it.

Needs the REST Control extension (dpar39.vscode-rest-control), configured in
.vscode/settings.json to listen on port 37100. That extension exposes VS Code's
command palette over localhost HTTP, so an agent or script can drive the editor:

  workbench.action.terminal.new      new integrated terminal
  custom.runInTerminal               type a command into the active terminal
  workbench.action.terminal.killAll  close them all

Why not the CLI: `code`/`code-insiders` has no --command flag, so nothing outside
the editor can open a terminal without an extension like this.

  vscode-terminal.py --name serial -- python3 scripts/serial_loopback_test.py --repeat
  vscode-terminal.py --run "ls -l"     # into the active terminal
  vscode-terminal.py --kill-all
"""

from __future__ import annotations

import argparse
import json
import sys
import urllib.error
import urllib.request

DEFAULT_PORT = 37100
# Localhost HTTP to an extension in the same machine's editor: a response is
# immediate or the server is not there. 2 s is ~3 orders over the real latency.
# On expiry: report that VS Code is not listening and how to fix it.
TIMEOUT_S = 2.0


def send(port: int, command: str, args: list | None = None) -> str:
    body = json.dumps({"command": command, **({"args": args} if args else {})})
    req = urllib.request.Request(
        f"http://localhost:{port}", data=body.encode(),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=TIMEOUT_S) as resp:
        return resp.read().decode("utf-8", "replace").strip()


def probe(port: int) -> bool:
    """True only if REST Control answers 200 to a real command.

    VS Code holds several other localhost ports that reply 404 to anything; those
    must count as a miss. The probe command is harmless and idempotent - it moves
    editor focus and nothing else.
    """
    try:
        send(port, "workbench.action.focusActiveEditorGroup")
        return True
    except (urllib.error.HTTPError, urllib.error.URLError, OSError):
        return False


def listening_ports() -> list[int]:
    """Ports VS Code holds open, newest first, plus the configured default."""
    import re
    import subprocess
    out = subprocess.run(["ss", "-ltnp"], capture_output=True, text=True).stdout
    found = [int(m) for m in re.findall(r"127\.0\.0\.1:(\d+).*code-insiders", out)]
    return [DEFAULT_PORT, *range(37100, 37110), *found]


def scan() -> int | None:
    for port in dict.fromkeys(listening_ports()):
        if probe(port):
            return port
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=DEFAULT_PORT)
    ap.add_argument("--scan", action="store_true",
                    help="find the REST Control port instead of assuming it")
    ap.add_argument("--name", help="label the new terminal")
    ap.add_argument("--run", help="run this in the ACTIVE terminal (no new one)")
    ap.add_argument("--kill-all", action="store_true", help="close every terminal")
    ap.add_argument("cmd", nargs="*", help="command to run in a NEW terminal")
    args = ap.parse_args()

    if args.scan:
        found = scan()
        if found is None:
            print("REST Control is not answering on any VS Code port.", file=sys.stderr)
            print("Reload the window: Ctrl+Shift+P -> Developer: Reload Window",
                  file=sys.stderr)
            return 1
        print(f"REST Control on port {found}")
        args.port = found

    try:
        if args.kill_all:
            send(args.port, "workbench.action.terminal.killAll")
            print("all terminals closed")
            return 0

        if args.run:
            send(args.port, "custom.runInTerminal", [args.run])
            print(f"sent to active terminal: {args.run}")
            return 0

        send(args.port, "workbench.action.terminal.new")
        if args.name:
            send(args.port, "workbench.action.terminal.renameWithArg",
                 [{"name": args.name}])
        if args.cmd:
            send(args.port, "custom.runInTerminal", [" ".join(args.cmd)])
            print(f"new terminal{' ' + args.name if args.name else ''}: {' '.join(args.cmd)}")
        else:
            print("new terminal opened")
        return 0

    except urllib.error.URLError as exc:
        print(f"VS Code is not listening on port {args.port}: {exc.reason}",
              file=sys.stderr)
        print("Fix: install dpar39.vscode-rest-control, then reload the window "
              "(Ctrl+Shift+P -> Developer: Reload Window).", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
