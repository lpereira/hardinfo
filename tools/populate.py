#!/usr/bin/python

import requests
import json
import sys

for benchmark, results in json.load(sys.stdin).items():
  for result in results:
    print('Populating: %s, %s' % (benchmark, result['MachineId']))

    if not result.get('Legacy', True):
      if not 'PointerBits' in result: result['PointerBits'] = 64

    r = requests.post('http://localhost:1234/benchmark.json', json={benchmark: result})
    if not r.ok:
      raise IOError("%s %d (%s)" % (r.reason, r.status_code, r.text.strip()))

