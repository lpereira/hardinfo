#!/bin/bash

echo "Updating data included in distribution.."

wget -O ../data/pci.ids https://pci-ids.ucw.cz/v2.2/pci.ids
wget -O ../data/usb.ids http://www.linux-usb.org/usb.ids
wget -O ../data/benchmark.json https://hardinfo2.org/benchmark.json

echo "Done..."


