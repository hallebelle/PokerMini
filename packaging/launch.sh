#!/bin/sh
cd "$(dirname "$0")"
export LD_LIBRARY_PATH=/lib:/config/lib:/customer/lib:/mnt/SDCARD/miyoo/lib:$LD_LIBRARY_PATH
./pokerqueen
