#!/bin/sh

export DISPLAY=:0
export XAUTHORITY=/run/user/1000/gdm/Xauthority
export XDG_RUNTIME_DIR=/run/user/1000
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus

/usr/bin/onboard &
ONBOARD_PID=$!

cleanup()
{
    kill "$ONBOARD_PID" 2>/dev/null
    wait "$ONBOARD_PID" 2>/dev/null
}

trap cleanup EXIT INT TERM

sleep 2

/usr/bin/zenity \
    --password \
    --title="KRAFTEK BMS - Authentication Required" \
    --text="Enter your Linux administrator password to shut down the Jetson TX2 NX."

exit $?
