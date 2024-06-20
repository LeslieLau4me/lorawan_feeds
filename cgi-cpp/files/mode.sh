#!/bin/sh

. /usr/share/libubox/jshn.sh

wm=NONE

load_lora_mode()
{
	json_init
	json_load_file /etc/lorawan_workmode/lorawan_workmode.conf
	json_get_var wm work_mode
	echo $wm
}

set_lora_mode()
{
	json_init
	json_add_string  'work_mode' $1
	json_dump  > /etc/lorawan_workmode/lorawan_workmode.conf
	echo "set $1"
}

stop_lora_mode()
{
	lora_pkt_fwd=$(ps | grep lora_pkt_fwd | grep -v "grep" | wc -l)
	if [ $lora_pkt_fwd = "1" ]; then
		echo "lora_pkt_fwd running, force stop"
		/etc/init.d/packet_forwarder stop
	fi
	station=$(ps | grep station | grep -v "grep" | wc -l)
	if [ $station = "1" ]; then
		echo "station running, force stop"
		/etc/init.d/basicstation stop
	fi
	bridge=$(ps | grep lora-gateway-bridge | grep -v "grep" | wc -l)
	if [ $bridge = "1" ]; then
		echo "LoRa gateway bridge running, force stop"
		/etc/init.d/lora-gateway-bridge stop
	fi
}

launch_mode()
{
	case "$wm" in
		PKFD)
			echo "loading packet_forwarder"
			stop_lora_mode
			sleep 1s
			/etc/init.d/packet_forwarder start

			;;
		BAST)
			echo "loading basic station"
			stop_lora_mode
			sleep 1s
			/etc/init.d/basicstation start
			;;
		BRDG)
			echo "loading gateway bridge"
			stop_lora_mode
			sleep 1s
			/etc/init.d/packet_forwarder start
			/etc/init.d/lora-gateway-bridge start

			;;
		*)
			;;
	esac
}

start() {
	load_lora_mode
	launch_mode
}

stop() {
	load_lora_mode
	case "$wm" in
		PKFD)
			echo "stop packet_forwarder"
			/etc/init.d/packet_forwarder stop

			;;
		BAST)
			echo "stop basic station"

			/etc/init.d/basicstation stop
			;;
		BRDG)
			echo "stop gateway bridge"
			/etc/init.d/packet_forwarder stop
			/etc/init.d/lora-gateway-bridge stop

			;;
		*)
			;;
	esac
}

restart() {
	stop
	start
}



case "$1" in
    start)
       start
       ;;
    stop)
       stop
       ;;
    restart)
       stop
       start
       ;;
	packet_forwarder)
		if [ "$2" = "start" ]; then
			set_lora_mode "PKFD"
			start
		fi
		;;

	bridge)
		if [ "$2" = "start" ]; then
			set_lora_mode "BRDG"
			start
		fi
		;;

	basicstation)
		if [ "$2" = "start" ]; then
			set_lora_mode "BAST"
			start
		fi
		;;

    *)
       echo "Usage1: $0 {start|stop|restart}"
	   echo "Usage2: $0 {packet_forwarder|bridge|basicstation} {start}"
esac

exit 0

