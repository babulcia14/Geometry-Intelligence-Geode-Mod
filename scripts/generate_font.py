"""Build a Polish/English BMFont atlas from the OFL-licensed Noto Sans font."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw, ImageFont

p = argparse.ArgumentParser()
p.add_argument("font")
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
font = ImageFont.truetype(args.font, 28)
codes = list(range(32, 127)) + list(range(160, 384)) + [8211, 8212, 8216, 8217, 8220, 8221, 8222, 8226, 8230, 8364, 8722, 65533]
atlas = Image.new("RGBA", (1024, 1024))
draw = ImageDraw.Draw(atlas)
rows = []
x, y, row_height = 2, 2, 0
ascent, descent = font.getmetrics()
for code in codes:
    c = chr(code)
    box = font.getbbox(c)
    width, height = max(1, box[2] - box[0]), max(1, box[3] - box[1])
    if x + width + 2 > atlas.width:
        x, y, row_height = 2, y + row_height + 4, 0
    if y + height + 2 > atlas.height:
        raise ValueError("Font atlas overflow")
    draw.text((x - box[0], y - box[1]), c, font=font, fill="white")
    rows.append(f"char id={code} x={x} y={y} width={width} height={height} xoffset={box[0]} yoffset={box[1]} xadvance={round(font.getlength(c))} page=0 chnl=15")
    x += width + 4
    row_height = max(row_height, height)
atlas.save(root / "resources/gi-font.png")
header = f'info face="GI Noto Sans" size=28 bold=0 italic=0 charset="" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1\ncommon lineHeight={ascent+descent} base={ascent} scaleW=1024 scaleH=1024 pages=1 packed=0\npage id=0 file="gi-font.png"\nchars count={len(rows)}\n'
(root / "resources/gi-font.fnt").write_text(header + "\n".join(rows) + "\n", encoding="utf-8")
print(f"Generated {len(codes)} glyphs")
