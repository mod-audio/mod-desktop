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
  "zip_exclude_packages": ["PIL","modtools"],
  "replace_paths": [["*",f".{s}lib{s}"]],
  "build_exe": f".{s}build-screenshot",
  "optimize": True,
}

exe_options = {
  "script": f".{s}mod-ui{s}modtools{s}pedalboard.py",
  "copyright": "Copyright (C) 2023 MOD Audio UG",
  "targetName": f"mod-screenshot{exe}",
}

setup(name = "mod-screenshot",
      version = "0.0.0",
      description = "MOD Pedalboard screenshot",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
