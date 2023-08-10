#!/bin/sh

echo 1 > /sys/class/leds/lte_rst/brightness
sleep 1
echo 0 > /sys/class/leds/lte_rst/brightness
echo "LTE module reset"

