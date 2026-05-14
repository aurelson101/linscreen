#!/usr/bin/env sh
set -eu

venv="${1:-.venv-tools}"

if [ ! -x "${venv}/bin/python" ]; then
    python3 -m venv "${venv}"
    "${venv}/bin/pip" install --upgrade pip cairosvg pillow
fi

"${venv}/bin/python" - <<'PY'
from pathlib import Path
import cairosvg
from PIL import Image

root = Path(".")
svg = root / "data/img/app/org.linscreen.LinScreen.svg"
app = root / "data/img/app"
h48 = root / "data/img/hicolor/48x48/apps"
h128 = root / "data/img/hicolor/128x128/apps"
hsvg = root / "data/img/hicolor/scalable/apps"

for path in (app, h48, h128, hsvg):
    path.mkdir(parents=True, exist_ok=True)

(app / "linscreen.svg").write_text(svg.read_text(encoding="utf-8"), encoding="utf-8")
(hsvg / "linscreen.svg").write_text(svg.read_text(encoding="utf-8"), encoding="utf-8")
(hsvg / "org.linscreen.LinScreen.svg").write_text(svg.read_text(encoding="utf-8"), encoding="utf-8")

mono_svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64"><rect x="5" y="8" width="54" height="40" rx="7" fill="#111827"/><rect x="10" y="13" width="44" height="30" rx="4" fill="#fff" fill-opacity=".92"/><circle cx="33" cy="34" r="13" fill="#111827"/><circle cx="33" cy="34" r="8" fill="#fff"/><circle cx="32.5" cy="31.5" r="3.5" fill="#111827"/><path fill="#111827" d="M25 53h14l2 5H23l2-5zM18 58h28v4H18z"/></svg>"""
(app / "linscreen.monochrome.svg").write_text(mono_svg, encoding="utf-8")
(app / "linscreen.mask.svg").write_text(mono_svg, encoding="utf-8")

outputs = {
    app / "org.linscreen.LinScreen.png": 256,
    app / "linscreen.png": 256,
    app / "org.linscreen.LinScreen-1024.png": 1024,
    h48 / "org.linscreen.LinScreen.png": 48,
    h48 / "linscreen.png": 48,
    h128 / "org.linscreen.LinScreen.png": 128,
    h128 / "linscreen.png": 128,
}

for out, size in outputs.items():
    cairosvg.svg2png(url=str(svg), write_to=str(out), output_width=size, output_height=size)

for name, source in [
    ("linscreen.monochrome.png", app / "linscreen.monochrome.svg"),
    ("linscreen.mask.png", app / "linscreen.mask.svg"),
    ("linscreen.monochrome-1024.png", app / "linscreen.monochrome.svg"),
]:
    size = 1024 if "1024" in name else 256
    cairosvg.svg2png(url=str(source), write_to=str(app / name), output_width=size, output_height=size)

ico_images = []
for size in (16, 24, 32, 48, 64, 128, 256):
    tmp = app / f".linscreen-{size}.png"
    cairosvg.svg2png(url=str(svg), write_to=str(tmp), output_width=size, output_height=size)
    ico_images.append(Image.open(tmp).convert("RGBA"))

ico_images[0].save(app / "linscreen.ico", sizes=[(im.width, im.height) for im in ico_images], append_images=ico_images[1:])

for tmp in app.glob(".linscreen-*.png"):
    tmp.unlink()
PY
