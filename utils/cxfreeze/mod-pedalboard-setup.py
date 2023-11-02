#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2012-2023 MOD Audio UG
# SPDX-License-Identifier: AGPL-3.0-or-later

# do an early import of everything we need
import os
import sys
import json
import aggdraw
import PIL

ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))

sys.path = [ROOT] + sys.path

from modtools.pedalboard import main
main()
