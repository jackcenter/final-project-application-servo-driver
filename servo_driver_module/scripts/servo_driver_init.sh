#!/bin/sh
argument="$1"
module=servo_driver_module
device=servo_driver

# Loads the module
start() {
    mode="666"
    set -e

    # Group: since distributions do it differently, look for wheel or use staff
    if grep -q '^staff:' /etc/group; then
        group="staff"
    else
        group="wheel"
    fi

    if [ -e ${module}.ko ]; then
        insmod $module.ko || exit 1
    else
        echo "Local file ${module}.ko not found, attempting to modprobe"
        modprobe ${module} || exit 1
    fi

    if [ -e /dev/${device} ]; then
        rm /dev/${device}
    fi

    major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)
    mknod /dev/${device} c $major 0
    chgrp $group /dev/${device}
    chmod $mode  /dev/${device}
}

# Unloads the module
stop() {
    rmmod $module || exit 1
    rm /dev/${device}
}

# Unloads then loads the module
restart() {
    stop
    start
}

# Prints if the module is loaded or not
status() {
    if lsmod | grep "$module" > /dev/null; then
        echo "$module is loaded"
    else
        echo "$module is not loaded"
    fi
}


if ! [ $# -eq 1 ]
then
    echo "Usage: servo_driver_init.sh {start|stop|restart|status}"
    exit 1
fi

cd $(dirname $0)/..
case "$argument" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: servo_driver_init.sh {start|stop|restart|status}"
        exit 1
        ;;
esac
