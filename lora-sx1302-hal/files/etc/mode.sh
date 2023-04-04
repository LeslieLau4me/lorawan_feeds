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
			/etc/init.d/packet_forwarder start
			/etc/init.d/lorabridge stop
			/etc/init.d/basicstation stop
			;;
		BAST)
			echo "loading basic station"
			/etc/init.d/packet_forwarder stop
                        /etc/init.d/lorabridge stop
                        /etc/init.d/basicstation start
			;;
		BRDG)
			echo "loading gateway bridge"
                        /etc/init.d/packet_forwarder stop
                        /etc/init.d/lorabridge start
                        /etc/init.d/basicstation stop
			;;
		*)
			;;
	esac
}

load_lora_mode
launch_mode
