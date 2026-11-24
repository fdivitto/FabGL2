#!/usr/bin/env bash
#
# Builds all (or selected) FabGL2 examples with ESP-IDF.
#
# Usage (from an ESP-IDF shell, i.e. after ". $IDF_PATH/export.sh"):
#
#   tools/build_examples.sh                  # build all examples
#   tools/build_examples.sh VGA/Sprites      # build only examples whose path contains "VGA/Sprites"
#
# Build directories are created under ./build_examples/ (examples folders are not modified).

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/build_examples"
FILTER="${1:-}"

mkdir -p "${OUT}"
ok=0; fail=0; failed=()

while IFS= read -r dir; do
  rel="${dir#${ROOT}/examples/}"
  [[ -n "${FILTER}" && "${rel}" != *"${FILTER}"* ]] && continue
  name="${rel//\//_}"
  bdir="${OUT}/${name}"
  echo "=== Building ${rel}"
  if idf.py -C "${dir}" -B "${bdir}" -D SDKCONFIG="${bdir}/sdkconfig" build > "${bdir}.log" 2>&1; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1)); failed+=("${rel}")
    echo "    FAILED (see ${bdir}.log)"
  fi
done < <(find "${ROOT}/examples" -name sdkconfig.defaults -not -path "*/build/*" -exec dirname {} \; | sort)

echo
echo "Succeeded: ${ok}   Failed: ${fail}"
for f in ${failed[@]+"${failed[@]}"}; do echo "  ${f}"; done
[[ ${fail} -eq 0 ]]
