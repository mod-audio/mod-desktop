#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------

options = {
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["mod","modtools"],
  "replace_paths": [["*",".\\lib\\"]],
  "build_exe": ".\\build",
  "optimize": True,
}

exe_options = {
  "script": ".\\utils\\mod-ui-wrapper.py",
  "copyright": "Copyright (C) 2023 MOD Audio UG",
  "targetName": "mod-ui.exe",
}

setup(name = "mod-ui",
      version = "0.0.0",
      description = "MOD Web interface",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
