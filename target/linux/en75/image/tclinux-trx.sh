#!/bin/sh

set -e

die() {
    echo "$1" >&2
    exit 1
}

to_hex() {
    hexdump -v -e '1/1 "%02x"'
}

from_hex() {
    perl -pe 's/\s+//g; s/(..)/chr(hex($1))/ge'
}

trx_crc32() {
    tmpfile=$(mktemp)
    outtmpfile=$(mktemp)
    cat "$1" "$2" > "$tmpfile"
    # We just need a CRC-32/JAMCRC of the concatnated files
    # There's no readily available tool for this, but zytrx does create one when
    # creating their TRX header, so we just use that.
    zytrx \
        -B NR7101 \
        -v x \
        -i "$tmpfile" \
        -o "$outtmpfile" >/dev/null
    dd if="$outtmpfile" bs=4 count=1 skip=3 | to_hex
    rm "$tmpfile" "$outtmpfile" >/dev/null
}

tclinux_trx_hdr() {
    hdrlen=256

    # TRX header magic
    printf '2RDH' | to_hex

    # Length of the header
    printf '%08x\n' "$hdrlen"

    # Length of header + content
    printf '%08x\n' $(($(stat -c '%s' "$1") + $(stat -c '%s' "$2") + $hdrlen))

    # crc32 of the content
    trx_crc32 "$1" "$2"

    # version
    echo "$3" | to_hex
    head -c "$((32 - $(echo "$3" | wc -c)))" /dev/zero | to_hex

    # customer version
    head -c 32 /dev/zero | to_hex

    # kernel length
    printf '%08x\n' $(stat -c '%s' "$1")

    # rootfs length
    printf '%08x\n' $(stat -c '%s' "$2")

    # romfile length (0)
    printf '00000000\n'

    # "model" (32 bytes of zeros)
    head -c 32 /dev/zero | to_hex

    # Load address (CONFIG_ZBOOT_LOAD_ADDRESS)
    printf '80020000\n'

    # "reserved" 128 bytes of zeros
    head -c 128 /dev/zero | to_hex
}

[ $# -eq 3 ] || die "SYNTAX: $0 <kernel lzma> <rootfs squashfs> <version string>"
kernel=$1
rootfs=$2
version=$3
which zytrx >/dev/null || die "zytrx not found in PATH $PATH"
[ -f "$kernel" ] || die "Kernel file not found: $kernel"
[ -f "$rootfs" ] || die "Rootfs file not found: $rootfs"
[ "$(echo "$version" | wc -c)" -lt 32 ] || die "Version string too long: $version"
tclinux_trx_hdr "$kernel" "$rootfs" "$version" | from_hex
cat "$kernel" "$rootfs"