#!/bin/sh
argument="$1"
module=servo_driver_module
device=servo_driver

if ! [ $# -eq 1 ]
then
    echo "Usage: servo_driver_load_unload.sh {load|unload}"
    exit 1
fi

cd $(dirname $0)/..
case "$argument" in
    load)
        mode="666"
        set -e

        # Group: since distributions do it differently, look for wheel or use staff
        if grep -q '^staff:' /etc/group; then
            group="staff"
        else
            group="wheel"
        fi

        if [ -e ${module}.ko ]; then
            echo "Loading local built file ${module}.ko"
            insmod $module.ko || exit 1
        else
            echo "Local file ${module}.ko not found, attempting to modprobe"
            modprobe ${module} || exit 1
        fi

        major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)
        rm /dev/${device}
        mknod /dev/${device} c $major 0
        chgrp $group /dev/${device}
        chmod $mode  /dev/${device}
        ;;
    unload)
        rmmod $module || exit 1
        rm /dev/${device}
        ;;
    *)
        echo "Usage: servo-driver-load-unload {load|unload}"
        exit 1
        ;;
esac
