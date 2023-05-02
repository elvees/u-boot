#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
# Copyright 2019-2022 RnD Center "ELVEES", JSC

set -o errexit
set -o pipefail

function help {
    echo "Description:
    Flash the MCom-03 SBL or SBIMG image to the SPI from OS running on the board.
    /etc/fw_env.config is used to detect U-Boot environment variables location.
    mtd-utils are required.
Usage:
    $(basename "$0") [OPTION] IMAGE
Arguments:
    IMAGE      SBL image file
Options:
    -h         print this message
    -o         provide offset in bytes from the beginning of SPI NOR"
}

function read_uboot_version {
    dd if=$DEV skip=$((OFFSET / BLOCK_SIZE)) bs=$BLOCK_SIZE \
        count=$((ALIGNED_IMAGE_SIZE / BLOCK_SIZE)) 2>/dev/null | \
        strings | grep -e 'U-Boot [0-9]'
}

while getopts "ho:" arg; do
    case "$arg" in
    h)
        help
        exit 0
    ;;
    o)
        OFFSET=$OPTARG
    ;;
    *)
        echo "Incorrect option"
        help
        exit 1
    ;;
    esac
done
shift $((OPTIND-1))
IMAGE=$1
if [[ -z "$IMAGE" ]]; then
    echo "SBL image is not specified"
    help
    exit 1
fi
shift
if [ ! -z "$@" ]; then
    echo "Too much arguments"
    help
    exit 1
fi
if [ ! -f "$IMAGE" ]; then
    echo "File '$IMAGE' not found"
    exit 1
fi

if [[ -z "$OFFSET" ]]; then
    OFFSET=0
fi

# Get environment parameters from a non-empty and non-commented line in the
# config file
ENV_CONFIG=/etc/fw_env.config
CONF_STR="$(grep -vE '(^[[:space:]]*#|^[[:space:]]*$)' $ENV_CONFIG | head -n1)"
readonly DEV=$(echo "$CONF_STR" | awk '{print $1}')

# TODO: Add lock check
readonly TEMPFILES=$(mktemp -d)
trap 'rm -rf $TEMPFILES' 0
trap 'echo "$LOG" >&2' ERR

readonly IMAGE_SIZE=$(wc -c "$IMAGE" | awk '{print $1}')
# Round up to mtd.erase_size
BLOCK_SIZE=$(mtd_debug info "$DEV" | grep -o "mtd.erasesize = [0-9]*")
BLOCK_SIZE=${BLOCK_SIZE##* }
# shellcheck disable=SC2017
readonly ALIGNED_IMAGE_SIZE=$(( (IMAGE_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE ))

# Check that offset is aligned to BLOCK_SIZE
readonly ALIGNED_IMAGE_OFFSET=$(( (OFFSET + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE ))
if [[ $OFFSET -ne $ALIGNED_IMAGE_OFFSET ]]; then
    echo "Image offset has to be aligned to '$BLOCK_SIZE'"
    exit 1
fi

# Check that image doesn't exceed DEV boundaries
MTD_SIZE=$(mtd_debug info "$DEV" | grep -o "mtd.size = [0-9]*")
MTD_SIZE=${MTD_SIZE##* }
readonly IMAGE_END=$(( OFFSET + IMAGE_SIZE ))

if [[ $IMAGE_END -gt $MTD_SIZE ]]; then
    echo "Image exceeds $DEV boundaries writing to $OFFSET. Operation aborted."
    exit 1
fi

echo "Flashing firmware. Do not terminate process!"
echo "Erasing old firmware..."
echo "U-Boot version on device: $(read_uboot_version)"
LOG=$(mtd_debug erase "$DEV" "$OFFSET" "$ALIGNED_IMAGE_SIZE" 2>&1)
echo "Writing firmware..."
LOG=$(mtd_debug write "$DEV" "$OFFSET" "$IMAGE_SIZE" "$IMAGE" 2>&1)

echo "Verifying firmware..."
echo "U-Boot version on device: $(read_uboot_version)"
CHECK_IMAGE=$TEMPFILES/check.image
LOG=$(mtd_debug read "$DEV" "$OFFSET" "$IMAGE_SIZE" "$CHECK_IMAGE" 2>&1)
if cmp "$IMAGE" "$CHECK_IMAGE" &> /dev/null; then
    echo "Flashing successful"
else
    echo "Flashing failed" >&2
    exit 1
fi
