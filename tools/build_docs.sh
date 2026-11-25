#!/usr/bin/env bash
#
# Generates the FabGL2 HTML documentation (API reference, User Guide and example pages) ready to be uploaded
# to https://www.fabglib.org.
#
# It regenerates docs/html from scratch with Doxygen (docs/Doxyfile) and checks the result: the build fails if
# Doxygen reports warnings (docs/doxygen_warnings.log) or if the main pages are missing.
# The content of docs/html is the site: upload it as it is (index.html at the site root).
#
# Requirements: doxygen, graphviz (dot) and python3 (used by docs/readme_filter.py for the example READMEs).
# On macOS: brew install doxygen graphviz
#
# Usage:
#
#   tools/build_docs.sh          # generate docs/html
#   tools/build_docs.sh -z       # also create build_docs/fabgl-docs-<version>.zip (content of docs/html)
#   tools/build_docs.sh -w       # do not fail on Doxygen warnings (they are still printed)
#   tools/build_docs.sh -s       # after generating, serve docs/html on http://localhost:8000 to check it
#
# Options can be combined, e.g. "tools/build_docs.sh -z -s".

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DOCS="${ROOT}/docs"
HTML="${DOCS}/html"
WARNLOG="${DOCS}/doxygen_warnings.log"

ZIP=0
ALLOWWARNINGS=0
SERVE=0

for arg in "$@"; do
  case "${arg}" in
    -z) ZIP=1 ;;
    -w) ALLOWWARNINGS=1 ;;
    -s) SERVE=1 ;;
    -h|--help)
      sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    *)
      echo "Unknown option: ${arg} (use -h for help)" >&2
      exit 1 ;;
  esac
done

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

# check the required tools
for tool in doxygen dot python3; do
  command -v "${tool}" >/dev/null 2>&1 || fail "'${tool}' not found. Install doxygen, graphviz and python3."
done
[ -x "${DOCS}/readme_filter.py" ] || fail "docs/readme_filter.py is missing or not executable"

VERSION="$(sed -n 's/^PROJECT_NUMBER *= *//p' "${DOCS}/Doxyfile" | tr -d '"[:space:]')"
echo "FabGL2 documentation ${VERSION}"
echo "  doxygen $(doxygen --version), $(dot -V 2>&1 | head -1)"

# generate (from scratch, so no stale pages remain)
rm -rf "${HTML}"
LOG="$(mktemp -t fabgl_doxygen.XXXXXX)"
echo "Running Doxygen..."
( cd "${DOCS}" && doxygen Doxyfile ) >"${LOG}" 2>&1
RC=$?
if [ ${RC} -ne 0 ]; then
  tail -30 "${LOG}" >&2
  fail "Doxygen failed (exit code ${RC}), full output in ${LOG}"
fi
rm -f "${LOG}"

# warnings
if [ -s "${WARNLOG}" ]; then
  echo
  echo "Doxygen warnings ($(wc -l < "${WARNLOG}" | tr -d ' ') lines, see docs/doxygen_warnings.log):"
  head -40 "${WARNLOG}"
  echo
  [ ${ALLOWWARNINGS} -eq 1 ] || fail "fix the warnings above, or run with -w to ignore them"
fi

# sanity checks
for page in index.html guide.html guide_examples.html annotated.html; do
  [ -f "${HTML}/${page}" ] || fail "docs/html/${page} was not generated"
done
EXPAGES=$(ls "${HTML}"/example__*.html 2>/dev/null | wc -l | tr -d ' ')
READMES=$(ls "${ROOT}"/examples/*/*/README.md 2>/dev/null | wc -l | tr -d ' ')
[ "${EXPAGES}" -eq "${READMES}" ] || fail "${READMES} example READMEs but ${EXPAGES} example pages: add the missing READMEs to INPUT in docs/Doxyfile"

FILES=$(find "${HTML}" -type f | wc -l | tr -d ' ')
SIZE=$(du -sh "${HTML}" | cut -f1)
echo "Generated docs/html: ${FILES} files, ${SIZE}, ${EXPAGES} example pages"

# archive
if [ ${ZIP} -eq 1 ]; then
  command -v zip >/dev/null 2>&1 || fail "'zip' not found"
  mkdir -p "${ROOT}/build_docs"
  ARCHIVE="${ROOT}/build_docs/fabgl-docs-${VERSION}.zip"
  rm -f "${ARCHIVE}"
  ( cd "${HTML}" && zip -qr "${ARCHIVE}" . ) || fail "cannot create ${ARCHIVE}"
  echo "Archive: build_docs/$(basename "${ARCHIVE}") ($(du -h "${ARCHIVE}" | cut -f1))"
fi

echo
echo "Ready: upload the content of docs/html to the root of www.fabglib.org"

# local preview
if [ ${SERVE} -eq 1 ]; then
  echo "Serving docs/html on http://localhost:8000 (Ctrl+C to stop)"
  cd "${HTML}" && exec python3 -m http.server 8000
fi
