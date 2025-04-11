#!/bin/bash
module=servo_driver_module
device=/dev/servo_driver
init_script="../scripts/servo_driver_init.sh"
test_file="/tmp/test_file.txt" 

create_test_file() {
    echo "  Creating $test_file..."
    touch "$test_file"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create $test_file."
        exit 1
    fi
}

remove_test_file() {
    echo "  Removing $test_file..."
    rm -f "$test_file"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to remove $test_file."
        exit 1
    fi
}

start_module() {
    echo "  Loading the module..."
    bash "$init_script" start
    if ! lsmod | grep "$module" > /dev/null; then
        echo "Error: Module $module did not load."
        exit 1
    fi

    if [ ! -c "$device" ]; then
        echo "Error: Device $device did not load."
        exit 1
    fi
}

write_to_device() {
    echo "  Writing to $device..."
    echo "TEST" > "$device"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to write to $device."
        exit 1
    fi 
}

read_from_device() {
    echo "  Reading from the $device..."
    
    create_test_file
    cat "$device" > "$test_file"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to read from $device."
        remove_test_file
        exit 1
    fi

    echo "  Data read from $device: $(cat $test_file)"
    remove_test_file
}

stop_module() {
    echo "  Unloading the module..."
    bash "$init_script" stop
    if lsmod | grep "$module" > /dev/null; then
        echo "Error: Module $module did not unload."
        exit 1
    fi

    if [ -c "$device" ]; then
        echo "Error: Device $device did not unload."
        exit 1
    fi
}

echo "Starting test for $module..."
cd "$(dirname "$0")" || exit 1

start_module
write_to_device
read_from_device
stop_module

echo "Test Completed Successfully"