#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
# Copyright 2023-2026 RnD Center "ELVEES", JSC

set -euo pipefail

SCRIPT=$(basename "$0")

LOCK_FILE="/var/lock/.${SCRIPT%.*}.lock"
LOCK_FD=

FACTORY_DEV="mmcblk0boot0"
FACTORY_DIR="/media/mmc/factory"
FACTORY_FS_TYPE="ext4"
FACTORY_SETTINGS="uboot-factory.env"

MOUNT_RO=("ro" "ro,noload" "1")                            # mount r/o
MOUNT_RW=("rw" "rw,nosuid,nodev,noexec,relatime,sync" "0") # mount r/w
UMOUNT_RW=("" "" "0")                                      # unmount, keep device r/w

SAVED_MOUNT_STATE=()

function help {
    cat <<EOF
Usage: $SCRIPT [OPTIONS] COMMAND [NAME [VALUE]]
Setup factory settings.

Options:

  -h, --help          print this message and exit
  -d, --dev=DEV       set device name to store factory settings (default
                        device name: '$FACTORY_DEV')
  -i, --dir=DIR       set mount point directory for the factory settings
                        (default mount point directory: '$FACTORY_DIR')
Commands:

  format              Format partition
  mountrw             Mount partition in read-write mode
  mountro             Mount partition in read-only mode
  unmount             Unmount partition
  set NAME [VALUE]    Set/unset the NAME-VALUE pair.
                        If VALUE is not provided then the NAME-VALUE pair
                        will be deleted.
                        Note: Only NAME started with 'factory_*' is allowed.
  print [NAME]        Print NAME-VALUE pair.
                        If NAME is not provided then all NAME-VALUE pairs
                        will be printed.
  state               Display current state of device and mount point

Examples:

  $SCRIPT format
  $SCRIPT set factory_eth0_mac ce:97:87:33:89:ac
  $SCRIPT set factory_serial <serial-number>

  # Attention! Use with care
  $SCRIPT set factory_wp 1

  $SCRIPT mountrw
  # Add extra files and settings to factory partition
  $SCRIPT mountro
EOF
}

function error() {
    echo "ERROR: $*" >&2
}

function info() {
    echo "INFO: $*" >&2
}

function warn() {
    echo "WARNING: $*" >&2
}

function fatal() {
    error "$@"
    exit 1
}

function is_filesystem_exists() {
    [[ $(blkid -s TYPE -o value "/dev/$FACTORY_DEV") == "$FACTORY_FS_TYPE" ]]
}

function is_filesystem_mounted() {
    [[ $(findmnt -n -o SOURCE --target "$FACTORY_DIR") == "/dev/$FACTORY_DEV" ]]
}

function mount_restore_state() {
    if [[ ${#SAVED_MOUNT_STATE[@]} -ne 0 ]]; then
        mount_remount "${SAVED_MOUNT_STATE[@]}"
        SAVED_MOUNT_STATE=()
    fi
}

function mount_read_state() {
    local -n result=$1
    local cur_flag="ro" cur_mode cur_prot
    cur_prot="$(cat "/sys/block/$FACTORY_DEV/force_ro")"
    local re="/dev/$FACTORY_DEV on $FACTORY_DIR type $FACTORY_FS_TYPE (\(.*\))"
    cur_mode="$(mount | sed -n "s;$re;\1;p")"
    if [[ -z $cur_mode ]]; then
        cur_flag="none"
    elif [[ ",$cur_mode," == *",rw,"* ]]; then
        cur_flag="rw"
    fi
    # shellcheck disable=SC2034
    result=("$cur_flag" "$cur_mode" "$cur_prot")
}

function mount_remount() {
    [[ $# -ne 3 ]] && fatal "Unexpected number of arguments to mount_remount()"

    local cur_state=() set_flag=$1 set_mode=$2 set_prot=$3
    mount_read_state "cur_state"

    [[ ${#cur_state[@]} -ne 3 ]] && fatal "Failed to read current mount state"

    if [[ ${#SAVED_MOUNT_STATE[@]} -eq 0 ]]; then
        SAVED_MOUNT_STATE=("${cur_state[@]}")
        trap mount_restore_state EXIT
    fi
    local cur_flag="${cur_state[0]}" cur_prot="${cur_state[2]}"
    if [[ $set_prot != "$cur_prot" ]] && [[ -n $set_prot ]]; then
        echo "$set_prot" >"/sys/block/$FACTORY_DEV/force_ro"
    fi
    if [[ $set_flag != "$cur_flag" ]]; then
        if is_filesystem_mounted; then
            sync
            umount "$FACTORY_DIR"
        fi
        if [[ -n $set_mode ]]; then
            if ! is_filesystem_exists; then
                fatal "Failed to mount. The filesystem doesn't exist"
            fi
            mount -t "$FACTORY_FS_TYPE" -o "$set_mode" "/dev/$FACTORY_DEV" "$FACTORY_DIR"
            systemctl daemon-reload
        fi
    fi
    if is_filesystem_mounted; then
        SAVED_MOUNT_STATE=()
    fi
}

function factory_format() {
    [[ $# -ne 0 ]] && fatal "No arguments expected for 'format' command"

    mount_remount "${UMOUNT_RW[@]}"
    mkfs -t "$FACTORY_FS_TYPE" -F "/dev/$FACTORY_DEV"
    mount_remount "${MOUNT_RW[@]}"
    touch "$FACTORY_DIR/$FACTORY_SETTINGS"
}

function factory_mountrw() {
    [[ $# -ne 0 ]] && fatal "No arguments expected for 'mountrw' command"

    mount_remount "${MOUNT_RW[@]}"
    warn "Be careful, settings are now accessible for read/write!!!"
}

function factory_mountro() {
    [[ $# -ne 0 ]] && fatal "No arguments expected for 'mountro' command"

    mount_remount "${MOUNT_RO[@]}"
    info "Settings are now protected from write!!!"
}

function factory_unmount() {
    [[ $# -ne 0 ]] && fatal "No arguments expected for 'unmount' command"

    mount_remount "${UMOUNT_RW[@]}"
    SAVED_MOUNT_STATE=()
    info "Settings are now unmounted!!!"
}

function factory_set() {
    [[ $# -ne 1 && $# -ne 2 ]] && fatal "Unexpected number of arguments for 'set' command"

    if [[ ! $1 =~ ^factory_.+ ]]; then
        fatal "It is required that NAME starts with 'factory_*'"
    fi
    if [[ $# -eq 2 ]] && [[ $1 =~ ^factory_eth[0-9]*_mac$ ]]; then
        local RE='^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$'
        if [[ ! $2 =~ $RE ]]; then
            fatal "MAC-address $2 is incorrect. The following format is allowed:"$'\n' \
                $'\t'"XX:XX:XX:XX:XX:XX, where X is a hexadecimal number (0-9,a-f,A-F)"
        fi
    fi
    mount_remount "${MOUNT_RW[@]}"
    sed -i "/^$1=/d" "$FACTORY_DIR/$FACTORY_SETTINGS" # Try to delete line with factory setting
    if [[ $# -eq 2 ]]; then
        echo "$1=$2" >>"$FACTORY_DIR/$FACTORY_SETTINGS" # Set/Update factory setting
    fi
    sort -o "$FACTORY_DIR/$FACTORY_SETTINGS" "$FACTORY_DIR/$FACTORY_SETTINGS" # Keep sorted
}

function factory_print() {
    [[ $# -gt 1 ]] && fatal "Unexpected number of arguments for 'print' command"

    if ! is_filesystem_mounted; then
        mount_remount "${MOUNT_RO[@]}"
    fi
    if [[ $# -eq 0 ]]; then
        cat "$FACTORY_DIR/$FACTORY_SETTINGS"
    else
        grep -m 1 -F "$1=" "$FACTORY_DIR/$FACTORY_SETTINGS" || true
    fi
}

function factory_state() {
    [[ $# -ne 0 ]] && fatal "No arguments expected for 'state' command"

    local have_fs="NO" have_mount="NO" have_prot="UNKNOWN" cur_state=()
    if is_filesystem_exists; then
        have_fs="YES"
    fi
    if is_filesystem_mounted; then
        have_mount="YES"
    fi
    mount_read_state "cur_state"

    [[ ${#cur_state[@]} -ne 3 ]] && fatal "Failed to read current mount state"

    case "${cur_state[2]}" in
    "0")
        have_prot="NO"
        ;;
    "1")
        have_prot="YES"
        ;;
    *)
        have_prot="UNKNOWN-${cur_state[2]}"
        ;;
    esac
    echo "Factory settings device:"
    echo "  name:       '$FACTORY_DEV'"
    echo "  formatted:  $have_fs"
    echo "  protected:  $have_prot"
    echo "Mount point information:"
    echo "  path:       '$FACTORY_DIR'"
    echo "  mounted:    $have_mount"
    echo "  mode:       ${cur_state[0]^^}"
    echo "  flags:      '${cur_state[1]}'"
}

function execute_command() {
    [[ $# -lt 1 ]] && fatal "Unexpected number of arguments for execute_command()"

    local func="factory_$1"
    if [[ $(type -t "$func") != "function" ]]; then
        fatal "Unknown command '$1' (try: $SCRIPT -h)"
    fi
    declare -A modes=(
        ["factory_format"]="--exclusive"
        ["factory_mountrw"]="--exclusive"
        ["factory_mountro"]="--exclusive"
        ["factory_unmount"]="--exclusive"
        ["factory_set"]="--exclusive"
        ["factory_print"]="--shared"
        ["factory_state"]="--shared"
    )
    touch "$LOCK_FILE"
    exec {LOCK_FD}<>"$LOCK_FILE" # create file descriptor in $LOCK_FD over the lockfile.
    flock "${modes[$func]}" "$LOCK_FD"
    shift
    "$func" "$@"
    flock --unlock "$LOCK_FD"
}

while getopts ":d:hi:-:" OPT; do
    if [[ $OPT == "-" ]]; then
        OPT="${OPTARG%%=*}"
        OPTARG="${OPTARG#"$OPT"}"
        OPTARG="${OPTARG#=}"
    fi
    case "$OPT" in
    d | dev)
        [[ -z $OPTARG ]] && fatal "Option -d|--dev requires an argument"
        FACTORY_DEV="$OPTARG"
        ;;
    h | help)
        help
        exit 0
        ;;
    i | dir)
        [[ -z $OPTARG ]] && fatal "Option -i|--dir requires an argument"
        FACTORY_DIR="$OPTARG"
        ;;
    :)
        fatal "Option -$OPTARG requires an argument"
        ;;
    *)
        fatal "Unknown option: -$OPTARG"
        ;;
    esac
done
shift $((OPTIND - 1))
execute_command "$@"
