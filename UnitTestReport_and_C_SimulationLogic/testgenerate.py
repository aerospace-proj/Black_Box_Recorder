#!/usr/bin/env python3
"""
run_tests_and_report.py
=======================
Compile and run test_all.c, then generate a PDF report with the results.

Usage:
    python3 run_tests_and_report.py [--src <path_to_test_all.c>] [--out <report.pdf>]

Dependencies:
    pip install reportlab
"""

import subprocess
import sys
import os
import re
import argparse
from datetime import datetime

from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    HRFlowable, KeepTogether
)
from reportlab.lib.enums import TA_CENTER, TA_LEFT

# ──────────────────────────────────────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────────────────────────────────────

PASS_COLOR   = colors.HexColor("#2e7d32")
FAIL_COLOR   = colors.HexColor("#c62828")
INFO_COLOR   = colors.HexColor("#1565c0")
SECTION_BG   = colors.HexColor("#e3f2fd")
HEADER_BG    = colors.HexColor("#0d47a1")
SUMMARY_PASS = colors.HexColor("#e8f5e9")
SUMMARY_FAIL = colors.HexColor("#ffebee")
SUMMARY_MIXED= colors.HexColor("#fff8e1")

def compile_tests(src_path: str, binary_path: str) -> tuple[bool, str]:
    src_dir = os.path.dirname(os.path.abspath(src_path))

    EXCLUDE = {"sqlite3.c", "shell.c", "main.c", "database.c"}

    all_c_files = [
        os.path.join(src_dir, f)
        for f in os.listdir(src_dir)
        if f.endswith(".c") and f not in EXCLUDE
    ]

    if not all_c_files:
        return False, f"No .c files found in {src_dir}"

    cmd = [
        "gcc",
        *all_c_files,
        "-o", binary_path,
        "-lm",
        "-Wall",
        f"-I{src_dir}",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return False, result.stderr
    return True, ""


def run_tests(binary_path: str) -> tuple[int, str]:
    result = subprocess.run(
        [binary_path], capture_output=True, timeout=30
    )
    def decode(b: bytes) -> str:
        for enc in ("cp1252", "latin-1"):
            try:
                return b.decode(enc)
            except Exception:
                continue
        return b.decode("utf-8", errors="replace")

    stdout = decode(result.stdout) if result.stdout else ""
    stderr = decode(result.stderr) if result.stderr else ""
    return result.returncode, stdout + stderr


# ──────────────────────────────────────────────────────────────────────────────
# Output parsing
# ──────────────────────────────────────────────────────────────────────────────

def parse_output(raw: str) -> dict:
    data = {"sections": [], "summary": {"total": 0, "passed": 0, "failed": 0}}
    current_section = None

    for line in raw.splitlines():
        line = line.strip()

        section_match = re.match(r"[|¦]\s{2}(.+?)\s*[|¦]$", line)
        if section_match and "---" not in line and "===" not in line:
            name = section_match.group(1).strip()
            if name and not re.match(r"^[-=+]+$", name):
                current_section = {"name": name, "results": []}
                data["sections"].append(current_section)
            continue

        pass_match = re.match(r"\[PASS\]\s*(.*)", line)
        fail_match = re.match(r"\[FAIL\]\s*(.*)", line)
        info_match = re.match(r"\[INFO\]\s*(.*)", line)

        if pass_match:
            entry = {"status": "PASS", "message": pass_match.group(1)}
        elif fail_match:
            entry = {"status": "FAIL", "message": fail_match.group(1)}
        elif info_match:
            entry = {"status": "INFO", "message": info_match.group(1)}
        else:
            continue

        if current_section is None:
            current_section = {"name": "General", "results": []}
            data["sections"].append(current_section)
        current_section["results"].append(entry)

    total_match  = re.search(r"Total\s*:\s*(\d+)", raw)
    passed_match = re.search(r"Passed\s*:\s*(\d+)", raw)
    failed_match = re.search(r"Failed\s*:\s*(\d+)", raw)
    if total_match:
        data["summary"]["total"]  = int(total_match.group(1))
    if passed_match:
        data["summary"]["passed"] = int(passed_match.group(1))
    if failed_match:
        data["summary"]["failed"] = int(failed_match.group(1))

    return data


# ──────────────────────────────────────────────────────────────────────────────
# PDF generation
# ──────────────────────────────────────────────────────────────────────────────

def build_pdf(parsed: dict, output_path: str, run_date: str,
              compile_ok: bool, compile_error: str,
              return_code: int, raw_output: str):

    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=0.75 * inch,
        rightMargin=0.75 * inch,
        topMargin=0.75 * inch,
        bottomMargin=0.75 * inch,
    )

    styles = getSampleStyleSheet()

    title_style = ParagraphStyle(
        "ReportTitle", parent=styles["Title"],
        fontSize=22, textColor=colors.white,
        spaceAfter=4, alignment=TA_CENTER,
    )
    subtitle_style = ParagraphStyle(
        "Subtitle", parent=styles["Normal"],
        fontSize=10, textColor=colors.HexColor("#bbdefb"),
        alignment=TA_CENTER, spaceAfter=2,
    )
    section_style = ParagraphStyle(
        "SectionHeader", parent=styles["Heading2"],
        fontSize=11, textColor=colors.HexColor("#0d47a1"),
        spaceBefore=10, spaceAfter=4, leftIndent=4,
    )
    mono_style = ParagraphStyle(
        "Mono", parent=styles["Code"],
        fontSize=7.5, leading=10, leftIndent=8,
        textColor=colors.HexColor("#37474f"),
    )
    error_style = ParagraphStyle(
        "ErrorStyle", parent=styles["Normal"],
        fontSize=9, textColor=FAIL_COLOR,
        leading=13, leftIndent=8,
    )

    story = []

    # Header banner
    header_table = Table(
        [[Paragraph("Aircraft Flight Crash Simulator", title_style)],
         [Paragraph("Unit Test Report", subtitle_style)],
         [Paragraph(f"Generated: {run_date}", subtitle_style)]],
        colWidths=[doc.width],
    )
    header_table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), HEADER_BG),
        ("TOPPADDING",    (0, 0), (-1, -1), 10),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 10),
        ("LEFTPADDING",   (0, 0), (-1, -1), 14),
        ("RIGHTPADDING",  (0, 0), (-1, -1), 14),
        ("ROUNDEDCORNERS", [6]),
    ]))
    story.append(header_table)
    story.append(Spacer(1, 14))

    if not compile_ok:
        story.append(Paragraph("Compilation Failed", section_style))
        story.append(Paragraph(compile_error.replace("\n", "<br/>"), error_style))
        story.append(Spacer(1, 8))
        doc.build(story)
        return

    # Summary block
    s = parsed["summary"]
    total, passed, failed = s["total"], s["passed"], s["failed"]
    all_pass   = failed == 0 and total > 0
    bg_color   = SUMMARY_PASS if all_pass else (SUMMARY_FAIL if failed > 0 else SUMMARY_MIXED)
    verdict_text  = "ALL TESTS PASSED ✓" if all_pass else f"{failed} TEST(S) FAILED ✗"
    verdict_color = PASS_COLOR if all_pass else FAIL_COLOR

    verdict_style = ParagraphStyle(
        "Verdict", parent=styles["Normal"],
        fontSize=13, textColor=verdict_color,
        alignment=TA_CENTER, spaceBefore=4, spaceAfter=4,
    )
    count_style = ParagraphStyle(
        "Count", parent=styles["Normal"],
        fontSize=10, alignment=TA_CENTER,
        textColor=colors.HexColor("#424242"),
    )

    summary_table = Table(
        [[Paragraph(verdict_text, verdict_style)],
         [Paragraph(
             f"Total: {total} &nbsp;&nbsp;|&nbsp;&nbsp; "
             f"<font color='#{PASS_COLOR.hexval()[2:]}'>Passed: {passed}</font> &nbsp;&nbsp;|&nbsp;&nbsp; "
             f"<font color='#{FAIL_COLOR.hexval()[2:]}'>Failed: {failed}</font>",
             count_style
         )]],
        colWidths=[doc.width],
    )
    summary_table.setStyle(TableStyle([
        ("BACKGROUND",    (0, 0), (-1, -1), bg_color),
        ("TOPPADDING",    (0, 0), (-1, -1), 8),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
        ("LEFTPADDING",   (0, 0), (-1, -1), 12),
        ("RIGHTPADDING",  (0, 0), (-1, -1), 12),
        ("BOX", (0, 0), (-1, -1), 1, verdict_color),
        ("ROUNDEDCORNERS", [4]),
    ]))
    story.append(summary_table)
    story.append(Spacer(1, 16))

    # Per-section results
    if parsed["sections"]:
        story.append(HRFlowable(width="100%", thickness=1, color=colors.HexColor("#90caf9")))
        story.append(Spacer(1, 6))
        story.append(Paragraph("Test Details", section_style))
        story.append(Spacer(1, 4))

    for section in parsed["sections"]:
        sec_name = section["name"]
        results  = section["results"]

        sec_pass = sum(1 for r in results if r["status"] == "PASS")
        sec_fail = sum(1 for r in results if r["status"] == "FAIL")
        badge = f"[{sec_pass}/{len(results)}]" if results else ""
        badge_col = PASS_COLOR if sec_fail == 0 else FAIL_COLOR

        sec_header = Table(
            [[
                Paragraph(sec_name, ParagraphStyle(
                    "SecName", parent=styles["Normal"],
                    fontSize=10, textColor=colors.HexColor("#0d47a1"),
                    fontName="Helvetica-Bold"
                )),
                Paragraph(badge, ParagraphStyle(
                    "Badge", parent=styles["Normal"],
                    fontSize=9, textColor=badge_col,
                    fontName="Helvetica-Bold", alignment=1
                )),
            ]],
            colWidths=[doc.width * 0.82, doc.width * 0.18],
        )
        sec_header.setStyle(TableStyle([
            ("BACKGROUND",    (0, 0), (-1, -1), SECTION_BG),
            ("TOPPADDING",    (0, 0), (-1, -1), 5),
            ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
            ("LEFTPADDING",   (0, 0), (-1, 0),  8),
            ("RIGHTPADDING",  (-1, 0), (-1, 0), 8),
            ("VALIGN",        (0, 0), (-1, -1), "MIDDLE"),
        ]))

        rows = []
        for r in results:
            st, msg = r["status"], r["message"]
            if st == "PASS":
                icon, col = "&#10003;", PASS_COLOR
            elif st == "FAIL":
                icon, col = "&#10007;", FAIL_COLOR
            else:
                icon, col = "&#8505;", INFO_COLOR

            icon_para = Paragraph(
                f'<font color="#{col.hexval()[2:]}" size="10"><b>{icon}</b></font>',
                ParagraphStyle("Icon", parent=styles["Normal"], fontSize=10, alignment=TA_CENTER)
            )
            msg_para = Paragraph(
                msg, ParagraphStyle(
                    "Msg", parent=styles["Normal"],
                    fontSize=8.5, leading=12, textColor=colors.HexColor("#212121")
                )
            )
            rows.append([icon_para, msg_para])

        if rows:
            result_tbl = Table(rows, colWidths=[0.3 * inch, doc.width - 0.3 * inch])
            result_tbl.setStyle(TableStyle([
                ("TOPPADDING",    (0, 0), (-1, -1), 3),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
                ("LEFTPADDING",   (0, 0), (-1, -1), 6),
                ("RIGHTPADDING",  (0, 0), (-1, -1), 6),
                ("VALIGN",        (0, 0), (-1, -1), "MIDDLE"),
                ("ROWBACKGROUNDS", (0, 0), (-1, -1), [colors.white, colors.HexColor("#f5f5f5")]),
                ("BOX",      (0, 0), (-1, -1), 0.5, colors.HexColor("#e0e0e0")),
                ("LINEBELOW", (0, 0), (-1, -2), 0.3, colors.HexColor("#eeeeee")),
            ]))
            story.append(KeepTogether([sec_header, result_tbl]))
        else:
            story.append(sec_header)

        story.append(Spacer(1, 8))

    # Raw output
    story.append(HRFlowable(width="100%", thickness=1, color=colors.HexColor("#90caf9")))
    story.append(Spacer(1, 6))
    story.append(Paragraph("Raw Output", section_style))
    story.append(Spacer(1, 4))

    safe_raw = raw_output.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    for line in safe_raw.splitlines()[:300]:
        story.append(Paragraph(line if line.strip() else "&nbsp;", mono_style))

    if len(raw_output.splitlines()) > 300:
        story.append(Paragraph("... (output truncated to 300 lines)", mono_style))

    doc.build(story)


# ──────────────────────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────────────────────

def main():
    # Default src = test_all.c situé dans le même dossier que ce script
    script_dir   = os.path.dirname(os.path.abspath(__file__))
    default_src  = os.path.join(script_dir, "test_all.c")

    parser = argparse.ArgumentParser(description="Compile test_all.c and produce a PDF report.")
    parser.add_argument("--src", default=default_src,
                        help=f"Path to test_all.c (default: {default_src})")
    parser.add_argument("--out", default=None,
                        help="Output PDF path (default: test_report_<timestamp>.pdf)")
    args = parser.parse_args()

    src_path    = args.src
    timestamp   = datetime.now().strftime("%Y%m%d_%H%M%S")
    run_date    = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    binary_path = os.path.join(script_dir, f"test_runner_{timestamp}")
    pdf_path    = args.out or os.path.join(script_dir, f"test_report_{timestamp}.pdf")

    print(f"[*] Compiling: {src_path}")
    compile_ok, compile_error = compile_tests(src_path, binary_path)

    raw_output  = ""
    return_code = -1
    parsed      = {"sections": [], "summary": {"total": 0, "passed": 0, "failed": 0}}

    if compile_ok:
        print("[*] Running tests...")
        try:
            return_code, raw_output = run_tests(binary_path)
        except subprocess.TimeoutExpired:
            raw_output = "[ERROR] Test binary timed out after 30 seconds."
        finally:
            if os.path.exists(binary_path):
                os.remove(binary_path)

        parsed = parse_output(raw_output)
        s = parsed["summary"]
        print(f"[*] Results — Total: {s['total']}, Passed: {s['passed']}, Failed: {s['failed']}")
    else:
        print(f"[!] Compilation failed:\n{compile_error}")

    print(f"[*] Generating PDF: {pdf_path}")
    build_pdf(parsed, pdf_path, run_date,
              compile_ok, compile_error,
              return_code, raw_output)

    print(f"[+] Report saved to: {os.path.abspath(pdf_path)}")
    sys.exit(0 if return_code == 0 else 1)


if __name__ == "__main__":
    main()