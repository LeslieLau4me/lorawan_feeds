#!/bin/sh

echo 1 > /sys/class/gpio/lte_rst/value
sleep 1
echo 0 > /sys/class/gpio/lte_rst/value
echo "LTE module reset"

