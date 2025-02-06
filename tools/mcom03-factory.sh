#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
# Copyright 2023 RnD Center "ELVEES", JSC

set -o errexit
set -o pipefail

SCRIPT=$(basename "$0")

function help {
   cat <<EOF

Description:
    Utility to setup U-Boot factory settings.
Usage:
    $SCRIPT [OPTIONS] CMD [NAME [VALUE]]
Options:
    -h         Print this message and exit.
Commands:
    init             - Initialize the U-Boot factory settings file.
    unprotect        - Disable the forced read-only access.
    protect          - Enable the forced read-only access.
    set NAME [VALUE] - Set/update/delete the U-Boot factory settings NAME-VALUE pair.
                       If VALUE is not provided then the NAME-VALUE pair will be deleted.
                       Note!!! Only NAME started with 'factory_*' is allowed.
    print [NAME]     - Print the U-Boot factory settings NAME-VALUE pair.
                       If NAME is not provided then all NAME-VALUE pairs will be printed.
Example:
    $SCRIPT init
    $SCRIPT set factory_eth0_mac ce:97:87:33:89:ac
    $SCRIPT set factory_serial <serial-number>

    # Attention! Use with care
    $SCRIPT set factory_wp 1

    $SCRIPT unprotect
    # Add extra files and settings to factory partition
    $SCRIPT protect
EOF
}

# The path to the factory dir
FACTORY_DIR=/media/mmc/factory

# The path to the file with factory variables, used by U-Boot
FACTORY_SETTINGS=$FACTORY_DIR/uboot-factory.env

# The command to be done on the U-Boot factory settings
CMD=

# The name of a factory variable to be set
NAME=

# The value to be assigned to a factory variable NAME
VALUE=

function error {
    echo "$@" >&2
}

function fatal {
    error "$@"
    exit 1
}

function is_filesystem_exists {
    FS_TYPE=$(blkid -s TYPE -o value /dev/mmcblk0boot0)
    [[ "$FS_TYPE" == "ext4" ]]
}

function mount_protected {
    if mountpoint -q "$FACTORY_DIR"; then
        sync
        umount "$FACTORY_DIR"
    fi

    echo 1 > /sys/block/mmcblk0boot0/force_ro

    if is_filesystem_exists; then
        mount -t ext4 -o ro,noexec,nodev,nosuid,sync /dev/mmcblk0boot0 "$FACTORY_DIR"
    fi
}

function mount_unprotected {
    if mountpoint -q "$FACTORY_DIR"; then
        sync
        umount "$FACTORY_DIR"
    fi

    echo 0 > /sys/block/mmcblk0boot0/force_ro

    if is_filesystem_exists; then
        mount -t ext4 -o rw,noexec,nodev,nosuid,sync /dev/mmcblk0boot0 "$FACTORY_DIR"
    fi
}

function init_factory {
    if mountpoint -q "$FACTORY_DIR"; then
        sync
        umount "$FACTORY_DIR"
    fi

    echo 0 > /sys/block/mmcblk0boot0/force_ro

    if is_filesystem_exists; then
        echo "INFO: Filesystem have been already initialized earlier"
    else
        mkfs.ext4 -F /dev/mmcblk0boot0
    fi

    mount -t ext4 -o rw,noexec,nodev,nosuid,sync /dev/mmcblk0boot0 "$FACTORY_DIR"
    touch "$FACTORY_SETTINGS"
}

function check_name {
    if [[ ! "$NAME" =~ ^factory_.+ ]]; then
        fatal "ERROR: It is required that NAME starts with 'factory_*'"
    fi
}

function check_mac {
    RE='^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$'
    if [[ "$NAME" =~ ^factory_eth[0-9]*_mac$ ]] && \
       [[ ! "$VALUE" =~ $RE ]]; then
        error "ERROR: MAC-address $VALUE is incorrect"
        error "MAC-address have to follow the format:"
        fatal -e "\tXX:XX:XX:XX:XX:XX, where X is a hexadecimal number (0-9,a-f,A-F)"
    fi
}

function check_value {
    check_mac
}

trap 'mount_protected' EXIT

while getopts "h" arg; do
    case "$arg" in
    h)
        help
        exit 0
    ;;
    *)
        fatal "ERROR: Unknown option '$arg'"
    ;;
    esac
done
shift $((OPTIND-1))

if [[ "$#" -lt 1 ]]; then
    fatal "ERROR: Incorrect number of arguments"
fi

CMD=$1
shift

# Do command with factory settings
case "$CMD" in
init)
    if [[ "$#" -ne 0 ]]; then
        fatal "ERROR: Incorrect number of arguments"
    fi
    init_factory
;;
unprotect)
    if [[ "$#" -ne 0 ]]; then
        fatal "ERROR: Incorrect number of arguments"
    fi
    trap - EXIT
    trap 'mount_unprotected' EXIT
;;
protect)
    if [[ "$#" -ne 0 ]]; then
        fatal "ERROR: Incorrect number of arguments"
    fi
;;
set)
    if [[ "$#" -ne 1 ]] && [[ "$#" -ne 2 ]]; then
        fatal "ERROR: Incorrect number of arguments"
    fi
    NAME=$1
    check_name
    mount_unprotected
    sed -i "/^$NAME=/d" "$FACTORY_SETTINGS"  # Try to delete line with factory setting
    if [[ "$#" -eq 2 ]]; then
        VALUE=$2
        check_value
        echo "$NAME=$VALUE" >> "$FACTORY_SETTINGS" # Set/Update factory setting
    fi
    sort -o "$FACTORY_SETTINGS" "$FACTORY_SETTINGS" # Keep sorted
;;
print)
    if [[ "$#" -ne 0 ]] && [[ "$#" -ne 1 ]]; then
        fatal "ERROR: Incorrect number of arguments"
    fi
    if [[ "$#" -eq 1 ]]; then
        NAME=$1
        grep -m 1 -F "$NAME=" "$FACTORY_SETTINGS" || true
    else
        cat "$FACTORY_SETTINGS"
    fi
;;
*)
    fatal "ERROR: Unknown command '$CMD'"
;;
esac
