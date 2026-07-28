#!/usr/bin/env python3
"""Generate design tokens for all UI frontends from design/theme.json.

Outputs:
  ui-qt/qml/Theme.qml     - QML singleton (Qt Quick frontend)
  shared/theme_tokens.h   - C header (LVGL frontend)
  ui-slint/ui/Theme.slint - Slint global (Slint frontend)

Run from the repository root:  python3 design/generate_tokens.py
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
THEME = json.loads((ROOT / "design" / "theme.json").read_text())

HEADER_NOTE = "This file is GENERATED from design/theme.json - do not edit by hand."


def snake(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def gen_qml() -> str:
    lines = [
        "// " + HEADER_NOTE,
        "pragma Singleton",
        "import QtQuick",
        "",
        "QtObject {",
    ]
    for k, v in THEME["colors"].items():
        lines.append(f'    readonly property color {k}: "{v}"')
    lines.append("")
    for k, v in THEME["font"].items():
        lines.append(f"    readonly property int font{k[0].upper()}{k[1:]}: {v}")
    lines.append("")
    for k, v in THEME["metric"].items():
        lines.append(f"    readonly property int {k}: {v}")
    lines.append("}")
    return "\n".join(lines) + "\n"


def gen_c_header() -> str:
    lines = [
        "/* " + HEADER_NOTE + " */",
        "#ifndef RAILDECK_THEME_TOKENS_H",
        "#define RAILDECK_THEME_TOKENS_H",
        "",
    ]
    for k, v in THEME["colors"].items():
        lines.append(f"#define RD_COLOR_{snake(k)} 0x{v.lstrip('#')}")
    lines.append("")
    for k, v in THEME["font"].items():
        lines.append(f"#define RD_FONT_{snake(k)} {v}")
    lines.append("")
    for k, v in THEME["metric"].items():
        lines.append(f"#define RD_METRIC_{snake(k)} {v}")
    lines += ["", "#endif /* RAILDECK_THEME_TOKENS_H */"]
    return "\n".join(lines) + "\n"


def gen_slint() -> str:
    lines = [
        "// " + HEADER_NOTE,
        "export global Theme {",
    ]
    for k, v in THEME["colors"].items():
        lines.append(f"    out property <color> {k}: {v};")
    lines.append("")
    for k, v in THEME["font"].items():
        lines.append(f"    out property <length> font{k[0].upper()}{k[1:]}: {v}px;")
    lines.append("")
    # Angles and physical values stay unitless numbers; pixel metrics are lengths.
    for k, v in THEME["metric"].items():
        if k.endswith("Deg") or k.endswith("Kmh"):
            lines.append(f"    out property <float> {k}: {v};")
        else:
            lines.append(f"    out property <length> {k}: {v}px;")
    lines.append("}")
    return "\n".join(lines) + "\n"


def main() -> None:
    qml_out = ROOT / "ui-qt" / "qml" / "Theme.qml"
    c_out = ROOT / "shared" / "theme_tokens.h"
    slint_out = ROOT / "ui-slint" / "ui" / "Theme.slint"
    qml_out.parent.mkdir(parents=True, exist_ok=True)
    c_out.parent.mkdir(parents=True, exist_ok=True)
    slint_out.parent.mkdir(parents=True, exist_ok=True)
    qml_out.write_text(gen_qml())
    c_out.write_text(gen_c_header())
    slint_out.write_text(gen_slint())
    print(f"wrote {qml_out}")
    print(f"wrote {c_out}")
    print(f"wrote {slint_out}")


if __name__ == "__main__":
    main()
