#!/bin/sh

DAEMON="aesdsocket"
PIDFILE="/var/run/$DAEMON.pid"

start() {
    printf 'Starting %s: ' "$DAEMON"
    start-stop-daemon -b -S -q  -x "/usr/bin/$DAEMON" \
            -- -d $PIDFILE
    status=$?
    if [ "$status" -eq 0 ]; then
            echo "OK"
    else
            echo "FAIL"
    fi
    return "$status"
}

stop() {
    printf 'Stopping %s: ' "$DAEMON"
    start-stop-daemon -K -s TERM -q -p "$PIDFILE"
    status=$?
    if [ "$status" -eq 0 ]; then
            rm -f "$PIDFILE"
            echo "OK"
    else
            echo "FAIL"
    fi
    return "$status"
}

restart() {
    stop
    sleep 1
    start
}

case "$1" in
    start|stop|restart)
            "$1";;
    reload)
            # Restart, since there is no true "reload" feature.
            restart;;
    *)
            echo "Usage: $0 {start|stop|restart|reload}"
            exit 1
esac
