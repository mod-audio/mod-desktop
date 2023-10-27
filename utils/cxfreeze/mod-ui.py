#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

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

options = {
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["mod","modtools"],
  "replace_paths": [["*",f".{s}lib{s}"]],
  "build_exe": f".{s}build-ui",
  "optimize": True,
}

exe_options = {
  "script": f".{s}utils{s}cxfreeze{s}mod-ui-setup.py",
  "copyright": "Copyright (C) 2023 MOD Audio UG",
  "targetName": f"mod-ui{exe}",
}

setup(name = "mod-ui",
      version = "0.0.0",
      description = "MOD Web interface",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
