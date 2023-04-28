#!/bin/sh /etc/rc.common

START=80
STOP=10

. /usr/share/libubox/jshn.sh

wm=NONE

load_lora_mode()
{
	json_init
	json_load_file /etc/lorawm.json
	json_get_var wm work_mode
	echo $wm
}

launch_mode()
{
	case "$wm" in
		PKFD)
			echo "loading packet_forwarder"
			/etc/init.d/lorabridge stop
			/etc/init.d/basicstation stop
			/etc/init.d/packet_forwarder stop
			sleep 1s
			/etc/init.d/packet_forwarder start

			;;
		BAST)
			echo "loading basic station"
			/etc/init.d/packet_forwarder stop
			/etc/init.d/lorabridge stop
			/etc/init.d/basicstation stop
			sleep 1s
			/etc/init.d/basicstation start
			;;
		BRDG)
			echo "loading gateway bridge"
			/etc/init.d/basicstation stop
			/etc/init.d/packet_forwarder stop
			/etc/init.d/lorabridge stop
			sleep 1s
			/etc/init.d/lorabridge start

			;;
		*)
			;;
	esac
}

load_lora_mode
launch_mode
