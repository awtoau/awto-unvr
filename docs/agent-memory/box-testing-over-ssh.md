---
name: box-testing-over-ssh
description: "woomera testing goes over SSH or a 2nd tio, never the shared serial console the user watches"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-17T21:58:57.148Z
---

Do automated woomera (UNVR) testing over **SSH** (box at 192.168.25.149, root/unvr), or a **second tio** on the same system — NOT by driving the shared serial-console tio socket the user watches.

**Why:** driving the shared tio (`scripts/_console.py`) injects my commands + the `@@P@@` scratch prompt into the console the user is watching, cluttering it. Repeatedly reloading `al_eth` over serial also wedged the NIC this session.

**How to apply:** prefer `ssh root@192.168.25.149` for command runs; if the serial console is required (U-Boot / pre-network / reboot-watch), spin up a *second* tio instance so the user's primary stays clean; always restore PS1 to the Fedora default when done. See [[no-plain-reboot-use-watchdog]] for the reboot mechanism.
