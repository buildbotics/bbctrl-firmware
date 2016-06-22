#!/bin/bash

### BEGIN INIT INFO
# Provides:          bbctl
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs $network
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Buildbotics Controller Web service
# Description:       Buildbotics Controller Web service
### END INIT INFO


DAEMON_NAME=bbctrl
DAEMON=/usr/local/bin/$DAEMON_NAME
DAEMON_OPTS=""
DAEMON_USER=root
DAEMON_DIR=/var/lib/$DAEMON_NAME
PIDFILE=/var/run/$DAEMON_NAME.pid
DEFAULTS=/etc/default/$DAEMON_NAME


. /lib/lsb/init-functions


if [ -e $DEFAULTS ]; then
    . $DEFAULTS
fi


do_start () {
    log_daemon_msg "Starting system $DAEMON_NAME daemon"
    mkdir -p $DAEMON_DIR &&
    start-stop-daemon --start --background --pidfile $PIDFILE --make-pidfile \
        --user $DAEMON_USER --chuid $DAEMON_USER --chdir $DAEMON_DIR \
        --startas $DAEMON -- $DAEMON_OPTS -l /var/log/$DAEMON_NAME.log
    log_end_msg $?
}


do_stop () {
    log_daemon_msg "Stopping system $DAEMON_NAME daemon"
    start-stop-daemon --stop --pidfile $PIDFILE --retry 10
    log_end_msg $?
}


case "$1" in
    start|stop) do_${1} ;;
    restart|reload|force-reload) do_stop; do_start ;;
    status)
        status_of_proc "$DAEMON_NAME" "$DAEMON" && exit 0 || exit $?
        ;;

    *)
        echo "Usage: /etc/init.d/$DAEMON_NAME {start|stop|restart|status}"
        exit 1
        ;;
esac
