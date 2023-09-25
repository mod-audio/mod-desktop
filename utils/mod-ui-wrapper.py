#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2012-2023 MOD Audio UG
# SPDX-License-Identifier: AGPL-3.0-or-later

# do an early import of everything we need
import os
import sys
import json
import uuid
from tornado import gen, iostream, web, websocket

ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
DATA_DIR = os.path.join(ROOT, 'data')

sys.path = [ROOT] + sys.path

os.environ['LV2_PATH'] = os.path.join(ROOT, 'plugins')
os.environ['MOD_DATA_DIR'] = DATA_DIR
os.environ['MOD_DEFAULT_PEDALBOARD'] = os.path.join(ROOT, 'default.pedalboard')
os.environ['MOD_DEV_ENVIRONMENT'] = '0'
os.environ['MOD_DEVICE_WEBSERVER_PORT'] = '8888'
os.environ['MOD_LOG'] = os.environ.get("MOD_LOG", '1')
os.environ['MOD_KEY_PATH'] = os.path.join(DATA_DIR, 'keys')
os.environ['MOD_HTML_DIR'] = os.path.join(ROOT, 'html')
os.environ['MOD_USER_FILES_DIR'] = os.path.join(DATA_DIR, 'user-files')

os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Loops'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Recordings'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Samples'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Tracks'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'MIDI Clips'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'MIDI Songs'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Reverb IRs'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Speaker Cabinets IRs'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Hydrogen Drumkits'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'SF2 Instruments'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'SFZ Instruments'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'Aida DSP Models'), exist_ok=True)
os.makedirs(os.path.join(DATA_DIR, 'user-files', 'NAM Models'), exist_ok=True)

# import webserver after setting up environment
from mod import webserver
webserver.run()
