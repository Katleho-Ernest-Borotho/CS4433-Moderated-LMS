#!/usr/bin/env bash
# Prepare an isolated release deployment folder on the Windows development PC.
# Run with MSYS2 UCRT64. Live database/attachments are NEVER bundled.
set -euo pipefail

if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
  echo "ERROR: Open MSYS2 UCRT64, not Git Bash." >&2
  exit 1
fi

for tool in cmake ninja windeployqt6 objdump; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "ERROR: Missing $tool" >&2
    exit 1
  fi
done

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD="/c/dev/cs4433-lms-release"
ROOT="${1:-/c/dev/cs4433-lms-packages}"
STAMP="$(date -u '+%Y%m%d_%H%M%S')"
DIST="$ROOT/ModeratedLMS_v1_0_$STAMP"

mkdir -p "$ROOT"
if [[ -e "$DIST" ]]; then
  echo "ERROR: Folder already exists: $DIST" >&2
  exit 1
fi

echo "Building clean Release in $BUILD..."
cmake -S "$SOURCE_DIR" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DLMS_ENABLE_COVERAGE=OFF \
  -DLMS_ENABLE_NFR_BENCHMARK=OFF \
  -DLMS_ENABLE_LOCAL_ADMIN_RECOVERY=OFF
cmake --build "$BUILD" --target ModeratedLMS ModeratedLMSRestore

mkdir -p "$DIST"
cp "$BUILD/ModeratedLMS.exe" "$DIST/"
cp "$BUILD/ModeratedLMSRestore.exe" "$DIST/"
cp "$BUILD/lms_schema.sql" "$DIST/"

echo "Deploying Qt 6 DLLs and platform plugins..."
windeployqt6 --release --dir "$DIST" "$DIST/ModeratedLMS.exe"
windeployqt6 --release --dir "$DIST" "$DIST/ModeratedLMSRestore.exe"

# Also collect the SQLite, libsodium, compiler and transitive UCRT64 DLLs
# that Qt's Windows deployment tool may not copy automatically.
declare -A available
for dll in /ucrt64/bin/*.dll; do
  base="${dll##*/}"
  available["${base,,}"]="$dll"
done

for pass in $(seq 1 20); do
  added=0
  while IFS= read -r -d '' binary; do
    while IFS= read -r dependency; do
      key="${dependency,,}"
      candidate="${available[$key]:-}"
      if [[ -n "$candidate" && ! -f "$DIST/${candidate##*/}" ]]; then
        cp "$candidate" "$DIST/"
        echo "Included DLL: ${candidate##*/}"
        ((added+=1))
      fi
    done < <(objdump -p "$binary" 2>/dev/null | awk '/DLL Name:/ {print $3}')
  done < <(find "$DIST" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)
  ((added==0)) && break
  if ((pass==20)); then
    echo "ERROR: Runtime dependency collection did not converge." >&2
    exit 1
  fi
done

if ! find "$DIST" -type f -iname 'qwindows.dll' -print -quit | grep -q .; then
  echo "ERROR: Qt Windows platform plugin qwindows.dll was not packaged." >&2
  exit 1
fi

cat > "$DIST/README_FIRST_RUN.txt" <<'README'
Moderated LMS v1.0 — Windows test deployment

1. Copy/extract this COMPLETE folder to a Windows laptop.
2. Run ModeratedLMS.exe. If no accounts exist, set up the first Administrator.
3. The app stores its own private database under Windows Local AppData.
4. lms_schema.sql must remain next to ModeratedLMS.exe.
5. Do not use ModeratedLMSRestore.exe until after a verified backup.
6. Test on a SECOND Windows laptop without relying on the developer's PATH.

This is a deployment-test build, NOT a signed production installer.
Review Qt, MSYS2 and other third-party redistribution licences before
distribution; retain necessary licence notices. Do not include live data.
README

echo
echo "Prepared deployment folder: $(cygpath -w "$DIST")"
echo "Next: test it on a second Windows laptop without MSYS2."
