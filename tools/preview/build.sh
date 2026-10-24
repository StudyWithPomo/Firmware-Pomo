#!/usr/bin/env bash
# Build and run the screenshot preview on a PC (MinGW-w64 or Linux g++), then convert to PNG.
#   tools/preview/build.sh            # from the repo root, after one `idf.py build`
# Needs: managed_components/lvgl__lvgl (fetched by idf.py) and build/config/sdkconfig.h, so
# LVGL here uses exactly the device's configuration.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/tools/preview"
OBJ="$HERE/build"
OUT="${1:-$HERE/out}"
LVGL="$ROOT/managed_components/lvgl__lvgl"
SDKCONFIG="$ROOT/build/config/sdkconfig.h"
JOBS="${JOBS:-8}"

[ -d "$LVGL" ] || { echo "Missing $LVGL: run idf.py build once first"; exit 1; }
[ -f "$SDKCONFIG" ] || { echo "Missing $SDKCONFIG: run idf.py build once first"; exit 1; }
mkdir -p "$OBJ/lvgl" "$OBJ/app" "$OUT"

# LVGL settings: every CONFIG_LV_* from the device build, plus warnings printed to stdout
{
    echo "#pragma once"
    grep '^#define CONFIG_LV_' "$SDKCONFIG"
    # The device allocates from the system heap; the preview uses a large LVGL pool instead,
    # so it can report how much each screen needs
    echo "#undef CONFIG_LV_USE_CLIB_MALLOC"
    echo "#define CONFIG_LV_USE_BUILTIN_MALLOC 1"
    echo "#undef CONFIG_LV_MEM_SIZE_KILOBYTES"
    echo "#define CONFIG_LV_MEM_SIZE_KILOBYTES 256"
    echo "#define CONFIG_LV_USE_LOG 1"
    echo "#define CONFIG_LV_LOG_LEVEL 2"
    echo "#define CONFIG_LV_LOG_PRINTF 1"
} > "$OBJ/pomo_kconfig.h.new"
# LVGL bakes its settings in at compile time, so a config change means recompiling it
if ! cmp -s "$OBJ/pomo_kconfig.h.new" "$OBJ/pomo_kconfig.h"; then
    rm -f "$OBJ"/lvgl/*.o
    mv "$OBJ/pomo_kconfig.h.new" "$OBJ/pomo_kconfig.h"
else
    rm -f "$OBJ/pomo_kconfig.h.new"
fi

DEFS=(-DLV_CONF_SKIP "-DLV_CONF_KCONFIG_EXTERNAL_INCLUDE=\"pomo_kconfig.h\"" "-DLV_ASSERT_HANDLER=abort();")
INCS=(-I"$OBJ" -I"$HERE/stubs" -I"$ROOT/main" -I"$ROOT/main/lvgl_hooks" -I"$LVGL")
CFLAGS=(-O1 -include "$HERE/stubs/compat.h" "${DEFS[@]}" "${INCS[@]}")

# LVGL itself (only rebuilt when the object is missing)
echo "Compiling LVGL..."
find "$LVGL/src" -name '*.c' | while read -r src; do
    obj="$OBJ/lvgl/$(echo "${src#$LVGL/src/}" | tr '/' '_' | sed 's/\.c$/.o/')"
    [ -f "$obj" ] || echo "$src $obj"
done | xargs -r -P "$JOBS" -n 2 sh -c 'gcc -c "$0" -o "$1" '"$(printf '%q ' "${CFLAGS[@]}")"' -w'

# The UI code, always rebuilt
echo "Compiling the UI..."
APP_C=("$ROOT"/main/ui/fonts/*.c)
APP_CPP=(
    "$ROOT"/main/ui/*.cpp
    "$ROOT"/main/ui/pet/*.cpp
    "$ROOT"/main/ui/screens/*.cpp
    "$ROOT"/main/app/session_core.cpp
    "$ROOT"/main/app/session.cpp
    "$ROOT"/main/utilities.cpp
    "$HERE"/sim_stubs.cpp
    "$HERE"/preview.cpp
)
rm -f "$OBJ"/app/*.o
echo "  (fonts)"
for src in "${APP_C[@]}"; do gcc -c "$src" -o "$OBJ/app/$(basename "$src").o" "${CFLAGS[@]}"; done
for src in "${APP_CPP[@]}"; do
    g++ -std=gnu++17 -Wall -c "$src" -o "$OBJ/app/$(basename "$src").o" "${CFLAGS[@]}"
done

echo "Linking..."
# Through a response file: the LVGL object list is too long for one Windows command line
(cd "$OBJ/lvgl" && ls *.o > ../lvgl_objs.txt && rm -f ../liblvgl.a && ar rcs ../liblvgl.a @../lvgl_objs.txt)
g++ -o "$OBJ/pomo_preview" "$OBJ"/app/*.o "$OBJ/liblvgl.a" -lm

echo "Rendering..."
"$OBJ/pomo_preview" "$OUT"
python "$HERE/to_png.py" "$OUT"
