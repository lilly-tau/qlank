#!/usr/bin/env bash

CFLAGS="-std=c90 -O3 -Wpedantic -Werror"

DEBUG=none
TEST=false

while getopts "dtv" OPT; do
	case $OPT in
	d)	DEBUG=gdb;;
	t)	TEST=true;;
	v)	DEBUG=valgrind;;
	esac
done

if [[ "$DEBUG" = none ]]; then
	CFLAGS="$CFLAGS -ffunction-sections -fdata-sections -Wl,--gc-sections"
else
	CFLAGS="$CFLAGS -g"
fi

if [[ -d build ]]; then
	rm -r build || exit 1
fi

mkdir build || exit 1
cc $CFLAGS -o build/qlank src/*.c || exit 1

if [[ "$TEST" = true ]]; then
	if [[ "$DEBUG" = gdb ]]; then
		gdb -ex "set args < test.qlank" build/qlank
	elif [[ "$DEBUG" = valgrind ]]; then
		valgrind --leak-check=full --tool=memcheck -s\
			build/qlank < test.qlank
	else
		build/qlank < test.qlank > build/test.wat
		wat2wasm build/test.wat -o test-env/main.wasm
	fi
fi
