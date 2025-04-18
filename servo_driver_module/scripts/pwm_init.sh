#!/bin/bash
set -u

argument="$1"

PWMCHIP="/sys/class/pwm/pwmchip0"
PWMCHANNEL0="pwm0"
PWMCHANNEL1="pwm1"

activate_channel() {
    CHANNEL=$1

    # Export the PWM channel if not already exported
    if [ ! -d "$PWMCHIP/$CHANNEL" ]; then
        echo "PWM channel $CHANNEL does not exist so it can't be deactivated."
        return
    fi

    echo 1 | tee "$PWMCHIP/$CHANNEL/enable" > /dev/null       # Enable PWM
}

deactivate_channel() {
    CHANNEL=$1

    # Export the PWM channel if not already exported
    if [ ! -d "$PWMCHIP/$CHANNEL" ]; then
        echo "$$CHANNEL" | tee "$PWMCHIP/export"
        sleep 0.1  # short delay to let the system create the pwm0 directory
    fi

    echo 0 | tee "$PWMCHIP/$CHANNEL/enable" > /dev/null       # Disable PWM
}

get_channel_status() {
    CHANNEL=$1

    if [ "$(cat $PWMCHIP/$CHANNEL/enable 2>/dev/null)" = "1" ]; then
        echo "PWM channel $CHANNEL is enabled."
    else
        echo "PWM channel $CHANNEL is disabled or not available."
    fi
}

start() {
    activate_channel "$PWMCHANNEL0"
    activate_channel "$PWMCHANNEL1"
}

stop() {
    deactivate_channel "$PWMCHANNEL0"
    deactivate_channel "$PWMCHANNEL1"
}

restart() {
    deactivate_channel "$PWMCHANNEL0"
    deactivate_channel "$PWMCHANNEL1"

    activate_channel "$PWMCHANNEL0"
    activate_channel "$PWMCHANNEL1"
}

status() {
    get_channel_status "$PWMCHANNEL0"
    get_channel_status "$PWMCHANNEL1"
}

if ! [ $# -eq 1 ]
then
    echo "Usage: pwm_init.sh {start|stop|restart|status}"
    exit 1
fi

if [ ! -d "$PWMCHIP" ]; then
    echo "PWM chip not available. Is the overlay enabled in /boot/config.txt?"
    exit 1
fi

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
        echo "Usage: pwm_init.sh {start|stop|restart|status}"
        exit 1
        ;;
esac
