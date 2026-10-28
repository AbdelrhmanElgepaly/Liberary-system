#!/usr/bin/env python3
"""
Generates every image used by the application (original artwork, no stock photos):
  resources/images/login_art.png   side panel of the login window
  resources/images/hero.png        dashboard banner
  resources/images/emblem.png      application emblem
  resources/icons/*.png            UI icons (white + dark variants)
  resources/resources.qrc          Qt resource list
  app.ico                          Windows executable icon
Run:  python tools/make_assets.py      (needs Pillow)
You can replace any PNG in resources/images with your own picture (same file name).
"""
import math
import os
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = os.path.join(ROOT, "resources", "images")
ICO = os.path.join(ROOT, "resources", "icons")
os.makedirs(IMG, exist_ok=True)
os.makedirs(ICO, exist_ok=True)

# ----------------------------------------------------------------- bookshelf art
BOOK_COLORS = [
    (112, 28, 36), (140, 44, 44), (38, 72, 112), (26, 52, 88), (34, 88, 70),
    (58, 104, 82), (96, 62, 36), (122, 84, 46), (74, 44, 90), (150, 112, 52),
    (176, 134, 60), (52, 52, 64), (88, 40, 58), (30, 64, 74),
]


def shade(c, f):
    return tuple(max(0, min(255, int(v * f))) for v in c)


def vertical_gradient(w, h, top, bottom):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        d.line([(0, y), (w, y)], fill=tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)))
    return img


def make_shelves(w, h, seed=7, rows=5):
    rnd = random.Random(seed)
    img = vertical_gradient(w, h, (30, 20, 14), (18, 12, 9))
    d = ImageDraw.Draw(img)
    row_h = h // rows
    plank = 16
    for r in range(rows):
        base = (r + 1) * row_h  # y of the shelf plank top
        x = rnd.randint(0, 10)
        while x < w:
            bw = rnd.randint(16, 40)
            bh = int((row_h - plank - 14) * rnd.uniform(0.66, 0.97))
            col = rnd.choice(BOOK_COLORS)
            col = shade(col, rnd.uniform(0.8, 1.15))
            top = base - bh
            lean = rnd.random() < 0.05
            if lean:
                top += 6
            d.rectangle([x, top, x + bw, base], fill=col)
            d.rectangle([x, top, x + 3, base], fill=shade(col, 1.35))       # left highlight
            d.rectangle([x + bw - 4, top, x + bw, base], fill=shade(col, 0.6))  # right shadow
            if rnd.random() < 0.7:  # gold bands
                for off in (0.14, 0.2) if rnd.random() < 0.5 else (0.78,):
                    yy = top + int(bh * off)
                    d.rectangle([x, yy, x + bw, yy + 3], fill=(212, 164, 70))
            if rnd.random() < 0.45:  # title label
                ly = top + int(bh * rnd.uniform(0.33, 0.5))
                d.rectangle([x + 5, ly, x + bw - 6, ly + int(bh * 0.14)], fill=(232, 214, 170))
            d.line([(x + bw + 1, top), (x + bw + 1, base)], fill=(10, 6, 4))
            x += bw + rnd.randint(1, 3)
            if rnd.random() < 0.06:  # gap between groups
                x += rnd.randint(30, 90)
        # shelf plank
        for i in range(plank):
            t = i / plank
            c = tuple(int(a + (b - a) * t) for a, b in zip((128, 92, 54), (60, 40, 22)))
            d.line([(0, base + i), (w, base + i)], fill=c)
        d.line([(0, base), (w, base)], fill=(176, 134, 84))
    return img


def vignette(img, strength=0.55):
    w, h = img.size
    mask = Image.new("L", (w, h), 0)
    md = ImageDraw.Draw(mask)
    md.ellipse([-w * 0.25, -h * 0.25, w * 1.25, h * 1.25], fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(min(w, h) * 0.25))
    dark = Image.new("RGB", (w, h), (8, 5, 3))
    out = Image.composite(img, dark, mask)
    return Image.blend(img, out, strength)


def warm_glow(img, cx, cy, radius, alpha=0.35):
    w, h = img.size
    glow = Image.new("L", (w, h), 0)
    ImageDraw.Draw(glow).ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=int(255 * alpha))
    glow = glow.filter(ImageFilter.GaussianBlur(radius * 0.5))
    warm = Image.new("RGB", (w, h), (255, 190, 100))
    return Image.composite(warm, img, glow)


def make_login_art():
    w, h = 880, 1120  # drawn tall, displayed cropped (cover)
    img = make_shelves(w, h, seed=11, rows=7)
    img = img.filter(ImageFilter.GaussianBlur(1.1))
    img = warm_glow(img, w * 0.5, h * 0.22, 380, 0.45)
    img = vignette(img, 0.7)
    # dark gradient toward the bottom so text stays readable
    grad = Image.new("L", (w, h))
    gd = ImageDraw.Draw(grad)
    for y in range(h):
        t = max(0.0, (y - h * 0.30) / (h * 0.70))
        gd.line([(0, y), (w, y)], fill=int(215 * (t ** 1.1)))
    img = Image.composite(Image.new("RGB", (w, h), (10, 6, 4)), img, grad)
    img.save(os.path.join(IMG, "login_art.png"), optimize=True)


def make_hero():
    w, h = 1800, 300
    big = make_shelves(w, h * 2, seed=23, rows=6).filter(ImageFilter.GaussianBlur(2.4))
    img = big.crop((0, h // 2, w, h // 2 + h))
    img = warm_glow(img, w * 0.82, h * 0.3, 300, 0.35)
    img.save(os.path.join(IMG, "hero.png"), optimize=True)


# --------------------------------------------------------------------- icon set
S = 8        # supersampling
G = 24       # icon grid (Feather-like)
PX = 48      # final icon size


def _pt(p):
    return (p[0] * PX * S / G, p[1] * PX * S / G)


def _line(d, pts, w, col, closed=False):
    pts = [_pt(p) for p in pts]
    if closed:
        pts = pts + [pts[0]]
    width = int(w * PX * S / G)
    d.line(pts, fill=col, width=width, joint="curve")
    r = width / 2
    for x, y in pts:
        d.ellipse([x - r, y - r, x + r, y + r], fill=col)


def _circle(d, c, r, w, col):
    x, y = _pt(c)
    rr = r * PX * S / G
    d.ellipse([x - rr, y - rr, x + rr, y + rr], outline=col, width=int(w * PX * S / G))


def _dot(d, c, r, col):
    x, y = _pt(c)
    rr = r * PX * S / G
    d.ellipse([x - rr, y - rr, x + rr, y + rr], fill=col)


def _rrect(d, box, rad, w, col):
    x0, y0 = _pt(box[:2])
    x1, y1 = _pt(box[2:])
    d.rounded_rectangle([x0, y0, x1, y1], radius=rad * PX * S / G, outline=col, width=int(w * PX * S / G))


def _arc_pts(cx, cy, rx, ry, a0, a1, n=24):
    return [(cx + rx * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             cy + ry * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


ICONS = {}


def icon(fn):
    ICONS[fn.__name__] = fn
    return fn


W = 2.0


@icon
def dashboard(d, c):
    for b in [(3.5, 3.5, 10, 10), (14, 3.5, 20.5, 10), (3.5, 14, 10, 20.5), (14, 14, 20.5, 20.5)]:
        _rrect(d, b, 1.6, W, c)


@icon
def book(d, c):
    _line(d, [(3, 5.5), (11, 7.5), (11, 20), (3, 18)], W, c, True)
    _line(d, [(21, 5.5), (13, 7.5), (13, 20), (21, 18)], W, c, True)


@icon
def students(d, c):
    _circle(d, (12, 8), 4, W, c)
    _line(d, _arc_pts(12, 21, 8, 7, 180, 360), W, c)


@icon
def swap(d, c):
    _line(d, [(4, 8), (20, 8)], W, c)
    _line(d, [(16, 4), (20, 8), (16, 12)], W, c)
    _line(d, [(20, 16), (4, 16)], W, c)
    _line(d, [(8, 12), (4, 16), (8, 20)], W, c)


@icon
def report(d, c):
    _line(d, [(6, 20), (6, 12)], 3.2, c)
    _line(d, [(12, 20), (12, 5)], 3.2, c)
    _line(d, [(18, 20), (18, 9)], 3.2, c)


@icon
def logout(d, c):
    _line(d, [(10, 4), (4, 4), (4, 20), (10, 20)], W, c)
    _line(d, [(9, 12), (21, 12)], W, c)
    _line(d, [(16, 7), (21, 12), (16, 17)], W, c)


@icon
def plus(d, c):
    _line(d, [(12, 5), (12, 19)], W, c)
    _line(d, [(5, 12), (19, 12)], W, c)


@icon
def minus(d, c):
    _line(d, [(5, 12), (19, 12)], W, c)


@icon
def edit(d, c):
    _line(d, [(4, 20), (5, 15), (16, 4), (20, 8), (9, 19)], W, c, True)
    _line(d, [(14, 6), (18, 10)], W, c)


@icon
def trash(d, c):
    _line(d, [(4, 7), (20, 7)], W, c)
    _line(d, [(9, 7), (9, 4), (15, 4), (15, 7)], W, c)
    _line(d, [(6, 7), (7, 21), (17, 21), (18, 7)], W, c)
    _line(d, [(10, 11), (10, 17)], W, c)
    _line(d, [(14, 11), (14, 17)], W, c)


@icon
def search(d, c):
    _circle(d, (11, 11), 6.5, W, c)
    _line(d, [(16, 16), (20.5, 20.5)], W, c)


@icon
def export(d, c):
    _line(d, [(12, 4), (12, 15)], W, c)
    _line(d, [(7, 10), (12, 15), (17, 10)], W, c)
    _line(d, [(4, 19.5), (20, 19.5)], W, c)


@icon
def alert(d, c):
    _line(d, [(12, 3.5), (22, 20), (2, 20)], W, c, True)
    _line(d, [(12, 9.5), (12, 14.5)], W, c)
    _dot(d, (12, 17.4), 1.2, c)


@icon
def layers(d, c):
    _line(d, [(12, 3), (22, 8), (12, 13), (2, 8)], W, c, True)
    _line(d, [(2, 12.5), (12, 17.5), (22, 12.5)], W, c)
    _line(d, [(2, 16.5), (12, 21.5), (22, 16.5)], W, c)


@icon
def check(d, c):
    _circle(d, (12, 12), 9, W, c)
    _line(d, [(7.5, 12.5), (10.8, 15.8), (16.5, 9)], W, c)


@icon
def cross(d, c):
    _circle(d, (12, 12), 9, W, c)
    _line(d, [(8.5, 8.5), (15.5, 15.5)], W, c)
    _line(d, [(15.5, 8.5), (8.5, 15.5)], W, c)


@icon
def info(d, c):
    _circle(d, (12, 12), 9, W, c)
    _line(d, [(12, 11), (12, 16.5)], W, c)
    _dot(d, (12, 7.6), 1.2, c)


@icon
def clock(d, c):
    _circle(d, (12, 12), 9, W, c)
    _line(d, [(12, 7), (12, 12), (15.5, 14)], W, c)


@icon
def user(d, c):
    _circle(d, (12, 8), 4, W, c)
    _line(d, _arc_pts(12, 21, 8, 7, 180, 360), W, c)


@icon
def chevron_down(d, c):
    _line(d, [(6, 9), (12, 15), (18, 9)], 2.4, c)


@icon
def chevron_up(d, c):
    _line(d, [(6, 15), (12, 9), (18, 15)], 2.4, c)


@icon
def lock(d, c):
    _rrect(d, (5, 11, 19, 21), 2, W, c)
    _line(d, _arc_pts(12, 11, 4.5, 6, 180, 360), W, c)


def make_icons():
    variants = {"white": (255, 255, 255, 255), "dark": (74, 61, 49, 255), "gold": (176, 106, 16, 255)}
    for name, fn in ICONS.items():
        for vname, col in variants.items():
            im = Image.new("RGBA", (PX * S, PX * S), (0, 0, 0, 0))
            fn(ImageDraw.Draw(im), col)
            im = im.resize((PX, PX), Image.LANCZOS)
            im.save(os.path.join(ICO, f"{name}_{vname}.png"))


def make_emblem():
    size = 512
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, size - 1, size - 1], radius=112, fill=255)
    grad = vertical_gradient(size, size, (236, 178, 84), (176, 106, 16)).convert("RGBA")
    im.paste(grad, (0, 0), mask)
    # book glyph in dark brown, drawn big
    big = Image.new("RGBA", (size * 2, size * 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    col = (40, 26, 12, 255)
    k = size * 2 / 24
    def P(x, y):
        return (x * k, y * k)
    cream = (255, 240, 205, 255)
    for pts in ([(3.5, 7.5), (11.45, 9.4), (11.45, 20.2), (3.5, 18.3)],
                [(20.5, 7.5), (12.55, 9.4), (12.55, 20.2), (20.5, 18.3)]):
        d.polygon([P(*p) for p in pts], fill=col)
    # page lines
    for i in range(3):
        y0 = 11.2 + i * 2.6
        d.line([P(5.4, y0 - 0.5), P(9.7, y0 + 0.4)], fill=cream, width=int(k * 0.42))
        d.line([P(18.6, y0 - 0.5), P(14.3, y0 + 0.4)], fill=cream, width=int(k * 0.42))
    cx, cy, R, r = 12, 4.6, 2.0, 0.85
    star = []
    for i in range(10):
        ang = math.radians(-90 + i * 36)
        rr = R if i % 2 == 0 else r
        star.append(P(cx + rr * math.cos(ang), cy + rr * math.sin(ang)))
    d.polygon(star, fill=(255, 238, 200, 255))
    big = big.resize((size, size), Image.LANCZOS)
    im = Image.alpha_composite(im, big)
    im.save(os.path.join(IMG, "emblem.png"))
    im.save(os.path.join(ROOT, "app.ico"), sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])


def write_qrc():
    lines = ['<RCC>', '  <qresource prefix="/">', '    <file>style.qss</file>']
    for folder, prefix in ((IMG, "images"), (ICO, "icons")):
        for f in sorted(os.listdir(folder)):
            if f.endswith(".png"):
                lines.append(f'    <file>{prefix}/{f}</file>')
    lines += ['  </qresource>', '</RCC>']
    with open(os.path.join(ROOT, "resources", "resources.qrc"), "w") as fh:
        fh.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    make_login_art()
    make_hero()
    make_icons()
    make_emblem()
    write_qrc()
    print("assets generated")
