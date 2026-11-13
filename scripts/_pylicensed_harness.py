# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Host harness for import-testing TrueNAS middleware against our truenas_pylicensed stub (#356).

- Importing this installs a last-resort finder: a module the host lacks becomes a permissive
  placeholder, if its top-level name is in $PYLICENSED_FAKE (comma list: what middleware imports).
- Never faked: truenas_pylicensed, middlewared, stdlib (a fake winreg hangs mimetypes).
- Same idea as middleware's own middlewared/fake_env.py, widened to every missing dep.
- Also a pytest plugin (`-p _pylicensed_harness`): runs `async def` tests under asyncio.run.
- Used by scripts/check-pylicensed-stub.py; not imported by anything else.
"""

from __future__ import annotations

import asyncio
import importlib.abc
import importlib.machinery
import inspect
import itertools
import os
import sys
import types

NEVER_FAKE = {"truenas_pylicensed", "middlewared"} | set(sys.stdlib_module_names)
FAKE_ONLY = {n for n in os.environ.get("PYLICENSED_FAKE", "").split(",") if n}
_ids = itertools.count(1 << 20)


class _PMeta(type):
    def __getattr__(cls, name):
        if name.startswith("__"):
            raise AttributeError(name)
        return _pclass(name)

    def __getitem__(cls, item):
        return cls

    def __iter__(cls):
        return iter(("placeholder",))

    def _same(cls, *a):
        return cls

    __or__ = __ror__ = __and__ = __rand__ = __invert__ = __neg__ = __add__ = (
        __radd__
    ) = _same


class _PBase(metaclass=_PMeta):
    def __init__(self, *a, **k):
        pass

    def __getattr__(self, name):
        if name.startswith("__"):
            raise AttributeError(name)
        return _pclass(name)()

    def __call__(self, *a, **k):
        # Bare decorator use returns the decorated function.
        if len(a) == 1 and not k and callable(a[0]):
            return a[0]
        return self

    def __mro_entries__(self, bases):
        return ()  # used as a base class: drop out, avoids metaclass conflicts

    def __iter__(self):
        return iter(("placeholder",))

    def __len__(self):
        return 0

    def __index__(self):
        return 0

    __int__ = __index__

    def __contains__(self, item):
        return False

    def __getitem__(self, item):
        return self

    def __enter__(self):
        return self

    def __exit__(self, *a):
        return False

    def _same(self, *a):
        return self

    __or__ = __ror__ = __and__ = __rand__ = __add__ = __radd__ = __sub__ = __mul__ = (
        _same
    )
    __truediv__ = __rtruediv__ = __mod__ = __invert__ = __neg__ = __lshift__ = (
        __rshift__
    ) = _same


def _pclass(name: str) -> type:
    return _PMeta(name, (_PBase,), {})


class _PInt(int):
    """ALL_CAPS constant: a distinct int, so IntEnum bodies built from them still construct."""

    def __getattr__(self, name):
        if name.startswith("__"):
            raise AttributeError(name)
        return _pclass(name)()


class _PModule(types.ModuleType):
    def __getattr__(self, name):
        if name.startswith("__"):
            raise AttributeError(name)
        if name.isupper():
            return _PInt(next(_ids))
        return _pclass(f"{self.__name__}.{name}")()


class _Finder(importlib.abc.MetaPathFinder, importlib.abc.Loader):
    faked: set[str] = set()

    def find_spec(self, name, path, target=None):
        top = name.split(".")[0]
        if top in NEVER_FAKE or top not in FAKE_ONLY:
            return None
        for f in sys.meta_path:
            if f is self:
                continue
            try:
                if f.find_spec(name, path, target) is not None:
                    return None
            except Exception:
                continue
        self.faked.add(top)
        return importlib.machinery.ModuleSpec(name, self, is_package=True)

    def create_module(self, spec):
        m = _PModule(spec.name)
        m.__path__ = []
        return m

    def exec_module(self, module):
        pass


FINDER = _Finder()
if not any(isinstance(f, _Finder) for f in sys.meta_path):
    sys.meta_path.append(FINDER)


def pytest_configure(config):
    config.addinivalue_line(
        "markers", "asyncio: run under asyncio.run by _pylicensed_harness"
    )


def pytest_pyfunc_call(pyfuncitem):
    if not inspect.iscoroutinefunction(pyfuncitem.obj):
        return None
    args = {a: pyfuncitem.funcargs[a] for a in pyfuncitem._fixtureinfo.argnames}
    asyncio.run(pyfuncitem.obj(**args))
    return True
