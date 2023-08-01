#!/bin/bash

turn_on_all_light() {
	echo "Turning on $1"
	echo 0 > /sys/class/leds/red/brightness
	echo 0 > /sys/class/leds/blue/brightness
	echo 0 > /sys/class/leds/green/brightness
}

turn_off_all_light() {
	echo "Turning off $1"
	echo 1 > /sys/class/leds/red/brightness
	echo 1 > /sys/class/leds/blue/brightness
	echo 1 > /sys/class/leds/green/brightness
}

turn_on_red_light() {
	echo "Turning on $1"
	echo 0 > /sys/class/leds/red/brightness
	echo 1 > /sys/class/leds/blue/brightness
	echo 1 > /sys/class/leds/green/brightness
}

turn_on_green_light() {
	echo "Turning on $1"
	echo 1 > /sys/class/leds/red/brightness
	echo 1 > /sys/class/leds/blue/brightness
	echo 0 > /sys/class/leds/green/brightness
}

turn_on_blue_light() {
	echo "Turning on $1"
	echo 1 > /sys/class/leds/red/brightness
	echo 0 > /sys/class/leds/blue/brightness
	echo 1 > /sys/class/leds/green/brightness
}

turn_on_yellow_light() {
	echo "Turning on $1"
	echo 0 > /sys/class/leds/red/brightness
	echo 1 > /sys/class/leds/blue/brightness
	echo 0 > /sys/class/leds/green/brightness
}

turn_on_purple_light() {
	echo "Turning on $1"
	echo 0 > /sys/class/leds/red/brightness
	echo 0 > /sys/class/leds/blue/brightness
	echo 1 > /sys/class/leds/green/brightness
}

if [ -z "$1" ]; then
	echo "Usage: sh /usr/bin/light_control.sh <light>"
	echo "light: all red green blue yellow purple off"
	exit 1
fi

light="$1"

if [ "$light" == "all" ]; then
	turn_on_all_light "all"
elif [ "$light" == "red" ]; then
	turn_on_red_light "red"
elif [ "$light" == "green" ]; then
	turn_on_green_light "green"
elif [ "$light" == "blue" ]; then
	turn_on_blue_light "blue"
elif [ "$light" == "yellow" ]; then
	turn_on_yellow_light "yellow"
elif [ "$light" == "purple" ]; then
	turn_on_purple_light "purple"
elif [ "$light" == "off" ]; then
	turn_off_all_light "off"
else	echo "Invalid light"
fi
