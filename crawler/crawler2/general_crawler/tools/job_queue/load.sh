#!/bin/bash

# xingxiaolong@oneboxtech.com
# 15010525040
# @2012-9-6  16:20:23 : init
# @2012-9-10 16:20:13 : 'reload' be used by "hourly indexing script" to start server
#                       so update 'reload' from 'killall' to 'stop then start' 
# @2012-09-17 : fix bug: "ui_server/bin/load.sh reload|restart" will stop load.sh self and dont start serivce,
#                       fix it
# @2012-09-20 : fix bug:ulimit -c unlimited
#
#


cd `dirname $0` || exit


PWD=`pwd`
SERVICE_DIR=`dirname ${PWD}`
SERVICE_NAME=`basename ${SERVICE_DIR}`

SUPERVISE='/usr/local/bin/supervise'

test_file_exists(){

	if [[ ! -f $1 ]]
	then
		echo "[error] file not exits : $1"
		exit;
	fi
}

test_file_exists ${SUPERVISE}

start_service(){

		echo "Starting ${SERVICE_NAME}: "
    ulimit -c unlimited
		${SUPERVISE}  ${PWD} 0</dev/null &>/dev/null &

		sleep 2
		echo -e  "\n#pstree\n"
		pstree crawler|grep ${SERVICE_NAME}
		echo "Start done!"

}
stop_service(){

		echo "Stoping ${SERVICE_NAME}"
		my_supervise_pid=`ps ax | grep 'supervise' | grep "${SERVICE_NAME}" | awk '!/grep/ {print $1}'`

		if [[ -z ${my_supervise_pid} ]]
		then
			echo "[error] ${SERVICE_NAME} is not runing!Cannot stop it!"
			exit;
		fi
		# kill this supervise
		kill ${my_supervise_pid}
		# kill ${SERVICE_NAME}
		ps ax | grep "${SERVICE_NAME}" | awk '!/grep/ {print $1}' | xargs kill

		sleep 2

		pstree crawler|grep ${SERVICE_NAME}	
		echo -e "Stop done!"

}

restart_service(){

		echo "Stoping ${SERVICE_NAME}"
		my_supervise_pid=`ps ax | grep 'supervise' | grep "${SERVICE_NAME}" | awk '!/grep/ {print $1}'`

		if [[ ! -z ${my_supervise_pid} ]]
		then
			kill ${my_supervise_pid}
		fi
		ps ax | grep "${SERVICE_NAME}" | awk '!/grep|load.sh/ {print $1}' | xargs kill

		sleep 2

		pstree crawler|grep ${SERVICE_NAME}
		echo -e "Stop done!"

		sleep 1
	
		echo "Starting ${SERVICE_NAME}: "
    ulimit -c unlimited
		${SUPERVISE}  ${PWD} 0</dev/null &>/dev/null &

		sleep 2
		echo -e  "\n#pstree\n"
		pstree crawler|grep ${SERVICE_NAME}
		echo "Start done!"



}

case "$1" in 

	start|load)
	
		start_service
		;;

	stop)

		stop_service
		;;

	restart|reload)
		
		restart_service
		
		;;

	*)
		echo "Usage : $0 {start|load  stop  restart|reload}"
		;;
esac
