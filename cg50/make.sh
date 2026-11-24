#!/usr/bin/env bash
# macOS / Linux build script (equivalent of make.bat).
# One-time toolchain setup:  ./tools/setup-toolchain.sh   (builds sh3eb-elf gcc, mkg3a and libfxcg into ~/prizm-toolchain)
set -euo pipefail
cd "$(dirname "$0")"
TC="${PRIZM_TOOLCHAIN:-$HOME/prizm-toolchain}"
export FXCGSDK="${FXCGSDK:-$TC/libfxcg}"
export PATH="$TC/prefix/bin:$PATH"
command -v sh3eb-elf-gcc >/dev/null || { echo "sh3eb-elf-gcc not found - run ./tools/setup-toolchain.sh first" >&2; exit 1; }
[ -f "$FXCGSDK/lib/libfxcg.a" ] || { echo "libfxcg not built - run ./tools/setup-toolchain.sh first" >&2; exit 1; }
make "$@"
echo "Built: $(pwd)/SquadRush.g3a"
