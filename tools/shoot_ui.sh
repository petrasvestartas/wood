#!/usr/bin/env bash
# A screenshot of the session viewer WITH its interface (the egui layer panel open, every row expanded, the command line), without a
# browser or a GPU: tools/ui_shot.rs runs the viewer's own State and egui panels in a winit window on Xvfb with Mesa's lavapipe,
# runs "Layers All;Element Interactions On;Element Attributes On;Arctic On;View Isometric;View Orthographic;Fit" one line a frame
# and reads the frame back at twice the interface density (3200 x 2000 device pixels for a 1600 x 1000 interface, as a browser at
# devicePixelRatio 2). ui_shot.rs includes a symlink mirror of the viewer's src/ with three native stubs patched (see its header);
# this script remakes that mirror when the viewer's sources change, so it follows them without rebuilding for every shot. Needs Xvfb and libxkbcommon-x11-0.
#   bash wood/tools/shoot_ui.sh <out.png> <scene.pb> [width height]     default 3200 2000
#   env: UI_SHOT_SCALE=2 (interface scale), UI_SHOT_VIEW=iso|iso_back, UI_SHOT_CMD="Layers All;...;Fit" (replaces the whole line)
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
VIEWER="${SESSION_VIEWER:-$WOOD/../session/session_viewer}"
HOST="$(rustc -vV | sed -n 's/host: //p')"
out="$1"
pb="$(realpath "$2")"
size="${3:-3200}x${4:-2000}"
[ -f /usr/share/vulkan/icd.d/lvp_icd.json ] && [ -z "${VK_ICD_FILENAMES:-}" ] && export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json

# the mirror of src/: symlinks, but state.rs, app/ui/mod.rs and app/feedback.rs patched copies
final="$VIEWER/examples/ui_shot_src"
mirror="$final.new"
rm -rf "$mirror"
mkdir -p "$mirror/app/ui"
for f in "$VIEWER"/src/*; do b="$(basename "$f")"; [ "$b" = app ] || [ "$b" = state.rs ] || ln -s "../../src/$b" "$mirror/$b"; done
for f in "$VIEWER"/src/app/*; do b="$(basename "$f")"; [ "$b" = ui ] || [ "$b" = feedback.rs ] || ln -s "../../../src/app/$b" "$mirror/app/$b"; done
for f in "$VIEWER"/src/app/ui/*; do b="$(basename "$f")"; [ "$b" = mod.rs ] || ln -s "../../../../src/app/ui/$b" "$mirror/app/ui/$b"; done
sed 's/^    fn publish(&self) {$/&\n        #[cfg(not(target_arch = "wasm32"))]\n        return;/' "$VIEWER/src/app/ui/mod.rs" > "$mirror/app/ui/mod.rs"
start="$(grep -n '^/// Replace the rows of the layers panel' "$VIEWER/src/app/feedback.rs" | cut -d: -f1)"
end="$(grep -n '^/// One measured load phase' "$VIEWER/src/app/feedback.rs" | cut -d: -f1)"
sed "${start},${end}{s/#\[cfg(target_arch = \"wasm32\")\]/#[cfg(any(target_arch = \"wasm32\", test))]/;s/#\[cfg(not(target_arch = \"wasm32\"))\]/#[cfg(not(any(target_arch = \"wasm32\", test)))]/}" \
    "$VIEWER/src/app/feedback.rs" > "$mirror/app/feedback.rs"
# logical_size in interface pixels: the device pixels over the window's scale factor
perl -0pe 's/(fn logical_size\(&self\).*?f64::from\(self\.gpu\.config\.width\))(,\s*f64::from\(self\.gpu\.config\.height\))/$1 \/ self.window.scale_factor()$2 \/ self.window.scale_factor()/s' \
    "$VIEWER/src/state.rs" > "$mirror/state.rs"
grep -q 'height) / self.window.scale_factor()' "$mirror/state.rs" || { echo "shoot_ui: State::logical_size not found to patch" >&2; exit 1; }
grep -q 'return;' "$mirror/app/ui/mod.rs" || { echo "shoot_ui: Ui::publish not found to patch" >&2; exit 1; }
# kept when nothing changed, so cargo does not rebuild the example for every shot
if [ -d "$final" ] && diff -rq --no-dereference "$mirror" "$final" > /dev/null 2>&1; then
    rm -rf "$mirror"
else
    rm -rf "$final"
    mv "$mirror" "$final"
fi

# the shot, built as a test of the example (the panels compile natively only under cfg(test))
cmp -s "$WOOD/tools/ui_shot.rs" "$VIEWER/examples/ui_shot.rs" || cp "$WOOD/tools/ui_shot.rs" "$VIEWER/examples/ui_shot.rs"
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
