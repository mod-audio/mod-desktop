#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

import os

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------

from sys import platform

if platform == 'win32':
  exe = '.exe'
  s = '\\'
else:
  exe = ''
  s = '/'

# ------------------------------------------------------------------------------------------------------------

# This is a publishable OAuth identifier, not a secret.  Bake the build value
# into the frozen executable so Finder-launched Desktop instances retain it.
# A runtime MOD_TONE3000_CLIENT_ID remains the higher-priority development
# override in mod/settings.py.
tone3000_client_id = os.environ.get("MOD_TONE3000_CLIENT_ID", "").strip()
if not tone3000_client_id:
  raise RuntimeError("MOD_TONE3000_CLIENT_ID must be non-empty when freezing mod-ui")

options = {
  "constants": [f"MOD_TONE3000_CLIENT_ID={tone3000_client_id!r}"] + (
    # "Report a problem" target for this Desktop build (its own forum topic, distinct from the OS one);
    # optional: without it mod-ui falls back to the Beta Testing category.
    [f"MOD_FEEDBACK_URL={os.environ['MOD_FEEDBACK_URL'].strip()!r}"] if os.environ.get("MOD_FEEDBACK_URL", "").strip() else []
  ),
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["Cryptodome","PIL","mod","modtools","PyQt5"],
  "replace_paths": [["*",f".{s}lib{s}"]],
  "build_exe": f".{s}build-ui",
  "optimize": True,
}

exe_options = {
  "script": f".{s}utils{s}cxfreeze{s}mod-ui-setup.py",
  "copyright": "Copyright (C) 2012-2024 MOD Audio UG",
  "targetName": f"mod-ui{exe}",
}

setup(name = "mod-ui",
      version = "0.0.0",
      description = "MOD Web interface",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
