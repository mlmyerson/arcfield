#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Install Zig 0.14.1 (or use clang/wasm-ld). Set ZIG to an explicit compiler path.
if [ -n "${ZIG:-}" ]; then
 "$ZIG" build-exe -target wasm32-freestanding -O ReleaseFast -fstrip -fno-entry --stack 65536 --export=tiles_ptr --export=schedule_ptr --export=bins_ptr --export=particles_ptr --export=configure --export=step --export=collision_count --export=exit_count --export=active_count --export=time_value --export=snapshot --import-symbols --initial-memory=1048576 --max-memory=1048576 -cflags -O3 -ffp-contract=off -fno-builtin -- native/engine.c -femit-bin=dist/compact-engine.wasm
else
 clang --target=wasm32 -O3 -ffp-contract=off -fno-builtin -nostdlib -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -Wl,--strip-all -Wl,--initial-memory=1048576 -Wl,--max-memory=1048576 native/engine.c -o dist/compact-engine.wasm
fi
