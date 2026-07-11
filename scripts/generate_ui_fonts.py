#!/usr/bin/env python3

from pathlib import Path
from math import ceil

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
FONT_PATH = Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
OUT_DIR = ROOT / "src" / "chinese"

ASCII = "".join(chr(i) for i in range(0x20, 0x7F))
BASIC_PUNCTUATION = (
    "，。！？；：、"
    "（）【】《》“”‘’"
    "￥…—·"
)
EXTRA_CHINESE_TEXT = (
    "饮零果其"
    "总价元价格已售罄库存"
    "会员购物车全部品食水果其他返回空支付成功失败"
    "摄像头预览请面向屏幕保持面部在预览框内等待画面"
    "登录注册人脸录入姓名请输入"
    "美式咖啡拿铁橙味汽水薯片巧克力黄油饼干"
    "苹果杯香蕉葡萄盒纸巾包湿巾数据线"
)


def strip_c_comments(text):
    out = []
    i = 0
    in_string = False
    in_char = False
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""

        if in_string:
            out.append(ch)
            if ch == "\\" and nxt:
                out.append(nxt)
                i += 2
                continue
            if ch == '"':
                in_string = False
            i += 1
            continue

        if in_char:
            out.append(ch)
            if ch == "\\" and nxt:
                out.append(nxt)
                i += 2
                continue
            if ch == "'":
                in_char = False
            i += 1
            continue

        if ch == '"':
            in_string = True
            out.append(ch)
            i += 1
            continue

        if ch == "'":
            in_char = True
            out.append(ch)
            i += 1
            continue

        if ch == "/" and nxt == "/":
            i += 2
            while i < len(text) and text[i] != "\n":
                i += 1
            out.append("\n")
            continue

        if ch == "/" and nxt == "*":
            i += 2
            while i + 1 < len(text) and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2
            continue

        out.append(ch)
        i += 1

    return "".join(out)


def collect_source_chinese():
    files = list((ROOT / "src" / "ui").rglob("*.c"))
    files.append(ROOT / "src" / "product" / "product_manager.c")

    chars = []
    for path in files:
        text = strip_c_comments(path.read_text(encoding="utf-8"))
        in_string = False
        escaped = False
        for ch in text:
            if in_string:
                if escaped:
                    escaped = False
                    continue
                if ch == "\\":
                    escaped = True
                    continue
                if ch == '"':
                    in_string = False
                    continue
                if "\u4e00" <= ch <= "\u9fff":
                    chars.append(ch)
            elif ch == '"':
                in_string = True

    return "".join(chars)


def pack_4bpp(pixels):
    packed = []
    for i in range(0, len(pixels), 2):
        hi = pixels[i] >> 4
        lo = pixels[i + 1] >> 4 if i + 1 < len(pixels) else 0
        packed.append((hi << 4) | lo)
    return packed


def c_array(name, values, per_line=12):
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(values), per_line):
        chunk = values[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{v:02x}" for v in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def c_u16_array(name, values, per_line=12):
    lines = [f"static const uint16_t {name}[] = {{"]
    for i in range(0, len(values), per_line):
        chunk = values[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def generate(size):
    font = ImageFont.truetype(str(FONT_PATH), size)
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    base_line = descent

    chars = sorted(
        set(ASCII + BASIC_PUNCTUATION + EXTRA_CHINESE_TEXT + collect_source_chinese()),
        key=ord
    )
    range_start = ord(chars[0])
    unicode_offsets = [ord(ch) - range_start for ch in chars]

    bitmap = []
    dsc = [{
        "bitmap_index": 0,
        "adv_w": 0,
        "box_w": 0,
        "box_h": 0,
        "ofs_x": 0,
        "ofs_y": 0,
    }]

    for ch in chars:
        bbox = font.getbbox(ch)
        advance = max(1, ceil(font.getlength(ch)))

        if ch == " ":
            dsc.append({
                "bitmap_index": len(bitmap),
                "adv_w": advance * 16,
                "box_w": 0,
                "box_h": 0,
                "ofs_x": 0,
                "ofs_y": 0,
            })
            continue

        left, top, right, bottom = bbox
        box_w = max(advance, ceil(right - min(left, 0)), 1)
        box_h = line_height

        img = Image.new("L", (box_w, box_h), 0)
        draw = ImageDraw.Draw(img)
        draw.text((-min(left, 0), 0), ch, font=font, fill=255)

        pixels = list(img.getdata())
        glyph_bitmap = pack_4bpp(pixels)
        dsc.append({
            "bitmap_index": len(bitmap),
            "adv_w": advance * 16,
            "box_w": box_w,
            "box_h": box_h,
            "ofs_x": 0,
            "ofs_y": -base_line,
        })
        bitmap.extend(glyph_bitmap)

    font_name = f"font_chinese_{size}"
    guard = f"FONT_CHINESE_{size}"

    dsc_lines = ["static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {"]
    for item in dsc:
        dsc_lines.append(
            "    {.bitmap_index = %d, .adv_w = %d, .box_w = %d, "
            ".box_h = %d, .ofs_x = %d, .ofs_y = %d},"
            % (
                item["bitmap_index"],
                item["adv_w"],
                item["box_w"],
                item["box_h"],
                item["ofs_x"],
                item["ofs_y"],
            )
        )
    dsc_lines.append("};")

    content = f'''/*******************************************************************************
 * Size: {size} px
 * Bpp: 4
 * Font: {FONT_PATH}
 * Generated by scripts/generate_ui_fonts.py.
 * Includes ASCII 0x20-0x7E and vending machine UI Chinese glyphs.
 ******************************************************************************/

#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("third_party/lvgl/lvgl.h")
#include "third_party/lvgl/lvgl.h"
#else
#error "Cannot find lvgl.h"
#endif

#ifndef {guard}
    #define {guard} 1
#endif

#if {guard}

#if __has_include("src/font/lv_font_fmt_txt.h")
#include "src/font/lv_font_fmt_txt.h"
#elif __has_include("third_party/lvgl/src/font/lv_font_fmt_txt.h")
#include "third_party/lvgl/src/font/lv_font_fmt_txt.h"
#else
#error "Cannot find lv_font_fmt_txt.h"
#endif

{c_array("glyph_bitmap", bitmap)}

{chr(10).join(dsc_lines)}

{c_u16_array("unicode_list_0", unicode_offsets)}

static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{
        .range_start = 0x{range_start:04x},
        .range_length = {unicode_offsets[-1] + 1},
        .glyph_id_start = 1,
        .unicode_list = unicode_list_0,
        .glyph_id_ofs_list = NULL,
        .list_length = {len(unicode_offsets)},
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

static lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {{
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
    .cache = &cache
}};

const lv_font_t {font_name} = {{
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {line_height},
    .base_line = {base_line},
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 1,
    .dsc = &font_dsc
}};

#endif
'''

    (OUT_DIR / f"font_chinese_{size}.c").write_text(content, encoding="utf-8")
    print(f"generated font_chinese_{size}: {len(chars)} glyphs, line_height={line_height}, base_line={base_line}")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    generate(12)
    generate(16)


if __name__ == "__main__":
    main()
