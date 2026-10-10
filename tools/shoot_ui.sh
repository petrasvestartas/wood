#!/usr/bin/env bash
# A screenshot of the session viewer WITH its interface (the egui layer panel open, every row expanded, the command line), without a
# browser or a GPU: tools/ui_shot.rs runs the viewer's own State and egui panels in a winit window on Xvfb with Mesa's lavapipe,
# runs "Layers All;Element Interactions On;Element Attributes On;Arctic On;View Isometric;View Orthographic;Fit" one line a frame
# and reads the frame back. ui_shot.rs includes a symlink mirror of the viewer's src/ with two native stubs patched (see its header);
# this script remakes that mirror every run, so it follows the viewer's current sources. Needs Xvfb and libxkbcommon-x11-0.
#   bash wood/tools/shoot_ui.sh <out.png> <scene.pb> [width height]
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
VIEWER="${SESSION_VIEWER:-$WOOD/../session/session_viewer}"
HOST="$(rustc -vV | sed -n 's/host: //p')"
out="$1"
pb="$(realpath "$2")"
size="${3:-1600}x${4:-1000}"
[ -f /usr/share/vulkan/icd.d/lvp_icd.json ] && [ -z "${VK_ICD_FILENAMES:-}" ] && export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json

# the mirror of src/: symlinks, but app/ui/mod.rs and app/feedback.rs patched copies
mirror="$VIEWER/examples/ui_shot_src"
rm -rf "$mirror"
mkdir -p "$mirror/app/ui"
for f in "$VIEWER"/src/*; do b="$(basename "$f")"; [ "$b" = app ] || ln -s "../../src/$b" "$mirror/$b"; done
for f in "$VIEWER"/src/app/*; do b="$(basename "$f")"; [ "$b" = ui ] || [ "$b" = feedback.rs ] || ln -s "../../../src/app/$b" "$mirror/app/$b"; done
for f in "$VIEWER"/src/app/ui/*; do b="$(basename "$f")"; [ "$b" = mod.rs ] || ln -s "../../../../src/app/ui/$b" "$mirror/app/ui/$b"; done
sed 's/^    fn publish(&self) {$/&\n        #[cfg(not(target_arch = "wasm32"))]\n        return;/' "$VIEWER/src/app/ui/mod.rs" > "$mirror/app/ui/mod.rs"
start="$(grep -n '^/// Replace the rows of the layers panel' "$VIEWER/src/app/feedback.rs" | cut -d: -f1)"
end="$(grep -n '^/// One measured load phase' "$VIEWER/src/app/feedback.rs" | cut -d: -f1)"
sed "${start},${end}{s/#\[cfg(target_arch = \"wasm32\")\]/#[cfg(any(target_arch = \"wasm32\", test))]/;s/#\[cfg(not(target_arch = \"wasm32\"))\]/#[cfg(not(any(target_arch = \"wasm32\", test)))]/}" \
    "$VIEWER/src/app/feedback.rs" > "$mirror/app/feedback.rs"
grep -q 'return;' "$mirror/app/ui/mod.rs" || { echo "shoot_ui: Ui::publish not found to patch" >&2; exit 1; }

# the shot, built as a test of the example (the panels compile natively only under cfg(test))
cp "$WOOD/tools/ui_shot.rs" "$VIEWER/examples/ui_shot.rs"
(cd "$VIEWER" && REGEN_PROTO=0 cargo test --release --example ui_shot --target "$HOST" --no-run) >&2
shot="$(ls -t "$VIEWER/target/$HOST/release/examples/"ui_shot-* | grep -v '\.d$' | head -1)"

# a virtual display when there is none
if [ -z "${DISPLAY:-}" ]; then
    Xvfb :97 -screen 0 3840x2400x24 > /dev/null 2>&1 &
    xvfb=$!
    trap 'kill $xvfb' EXIT
    export DISPLAY=:97
    sleep 1
fi

ppm="$(mktemp --suffix=.ppm)"
UI_SHOT_IN="$pb" UI_SHOT_OUT="$ppm" UI_SHOT_SIZE="$size" "$shot" --exact shot::ui_shot --nocapture --test-threads=1 >&2
convert "$ppm" -depth 8 "$out"
rm -f "$ppm"
