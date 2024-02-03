#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2012-2023 MOD Audio UG
# SPDX-License-Identifier: AGPL-3.0-or-later

# do an early import of everything we need
import os
import sys
import json
import uuid
import aggdraw
import PIL
from tornado import gen, iostream, web, websocket
from Cryptodome.Cipher import PKCS1_OAEP, AES
from Cryptodome.Hash import SHA1
from Cryptodome.PublicKey import RSA
from Cryptodome.Signature import pkcs1_15 as PKCS1_v1_5

ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
DATA_DIR = os.getenv('MOD_DATA_DIR', os.path.join(ROOT, 'data'))
DEVICE_DIR = os.path.join(DATA_DIR, 'device')

sys.path = [ROOT] + sys.path

if sys.platform == 'darwin':
    resdir = os.path.join(ROOT, '..', 'Resources')
else:
    resdir = ROOT

os.environ['MOD_APP'] = '1'
os.environ['MOD_DEFAULT_PEDALBOARD'] = os.path.join(resdir, 'default.pedalboard')
os.environ['MOD_DEV_ENVIRONMENT'] = '0'
os.environ['MOD_DEVICE_HOST_PORT'] = '18182'
os.environ['MOD_DEVICE_WEBSERVER_PORT'] = '18181'
os.environ['MOD_HARDWARE_DESC_FILE'] = os.path.join(resdir, 'mod-hardware-descriptor.json')
os.environ['MOD_HTML_DIR'] = os.path.join(resdir, 'html')
os.environ['MOD_IMAGE_VERSION_PATH'] = os.path.join(resdir, 'VERSION')
os.environ['MOD_KEYS_PATH'] = os.path.join(DATA_DIR, 'keys')
os.environ['MOD_LOG'] = os.environ.get("MOD_LOG", '1')
# MOD_MODEL_CPU
os.environ['MOD_MODEL_TYPE'] = "MOD Desktop"
os.environ['MOD_USER_FILES_DIR'] = os.path.join(DATA_DIR, 'user-files')
os.environ['MOD_USER_PEDALBOARDS_DIR'] = os.path.join(DATA_DIR, 'pedalboards')
os.environ['MOD_USER_PLUGINS_DIR'] = os.path.join(DATA_DIR, 'lv2')

def makedirs(path: str):
    try:
        os.makedirs(path, exist_ok=True)
    except OSError:
        pass

makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Loops'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Recordings'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Samples'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Audio Tracks'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'MIDI Clips'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'MIDI Songs'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Reverb IRs'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Speaker Cabinets IRs'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Hydrogen Drumkits'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'SF2 Instruments'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'SFZ Instruments'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'Aida DSP Models'))
makedirs(os.path.join(DATA_DIR, 'user-files', 'NAM Models'))

# fake device setup
makedirs(DEVICE_DIR)
#os.environ['MOD_API_KEY'] = os.path.join(resdir, '..', 'mod_api_key.pub')
os.environ['MOD_DEVICE_KEY'] = os.path.join(DEVICE_DIR, 'rsa')
os.environ['MOD_DEVICE_TAG'] = os.path.join(DEVICE_DIR, 'tag')
os.environ['MOD_DEVICE_UID'] = os.path.join(DEVICE_DIR, 'uid')

from datetime import datetime
from random import randint

if os.path.exists(DEVICE_DIR):
    if not os.path.isfile(os.environ['MOD_DEVICE_TAG']):
        with open(os.environ['MOD_DEVICE_TAG'], 'w') as fh:
            tag = 'MDS-{0}-0-00-000-{1}'.format(datetime.utcnow().strftime('%Y%m%d'), randint(9000, 9999))
            fh.write(tag)
    if not os.path.isfile(os.environ['MOD_DEVICE_UID']):
        with open(os.environ['MOD_DEVICE_UID'], 'w') as fh:
            uid = ':'.join(['{0}{1}'.format(randint(0, 9), randint(0, 9)) for i in range(0, 16)])
            fh.write(uid)
    if not os.path.isfile(os.environ['MOD_DEVICE_KEY']):
        try:
            key = RSA.generate(2048)
            with open(os.environ['MOD_DEVICE_KEY'], 'wb') as fh:
                fh.write(key.exportKey('PEM'))
            with open(os.environ['MOD_DEVICE_KEY'] + '.pub', 'wb') as fh:
                fh.write(key.publickey().exportKey('PEM'))
        except Exception as ex:
            print('Can\'t create a device key: {0}'.format(ex))

# import webserver after setting up environment
from mod import webserver
webserver.run()
