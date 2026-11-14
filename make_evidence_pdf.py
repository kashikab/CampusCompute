#!/usr/bin/env python3
"""Compile actual student screenshots into SRN.pdf; never synthesizes screenshots."""
from pathlib import Path
import argparse
import re
import sys

CAPTIONS = [
    "Ubuntu/WSL terminal shows my complete SRN in the prompt and the project working directory.",
    "GCC and Make are available in the Ubuntu environment.",
    "Project source files, input data, and Makefile are present.",
    "A clean compilation completes successfully from the provided C source files.",
    "The command-line help lists all independent features and valid arguments.",
    "FCFS shows its execution timeline and computed job metrics.",
    "Round Robin displays quantum 2, the timeline, and computed job metrics.",
    "Non-preemptive priority scheduling shows priorities and computed job metrics.",
    "All three policies are compared using the same input jobs.",
    "The concurrent bounded queue shows producer and consumer activity.",
    "All logger threads terminate cleanly with the expected event count.",
    "A child and parent exchange messages through POSIX shared memory created with shm_open.",
    "Named semaphores coordinate the exchange, and /dev/shm shows no leftover objects after cleanup.",
    "Banker's algorithm identifies a safe state and outputs a safe sequence.",
    "Banker's algorithm correctly identifies the unsafe input state.",
    "The parent reaps a normally completed worker with waitpid.",
    "The parent detects the worker's controlled, simulated failure status 42.",
    "RLIMIT_FSIZE limits file writing, and the child removes its temporary file.",
    "The program rejects invalid input with a useful error message.",
    "Cleanup verification shows no remaining processes, threads or program-created IPC objects.",
    "The integrated execution completes all demonstrations successfully.",
    "The program-generated log file exists and contains timestamped event entries.",
    "The final source, Makefile, and compiled executable are present.",
    "The submission ZIP listing and archive integrity test succeed.",
]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--srn",required=True,help="Your actual full SRN, used for the PDF filename")
    ap.add_argument("--dir",default="screenshots",help="Folder with S01.png to S24.png")
    args=ap.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]{5,32}",args.srn):
        ap.error("Use your real SRN, 5–32 letters/digits/hyphens/underscores")
    try:
        from reportlab.platypus import SimpleDocTemplate,Paragraph,Image,Spacer,PageBreak
        from reportlab.lib.styles import getSampleStyleSheet
        from reportlab.lib.utils import ImageReader
    except ImportError:
        sys.exit("Install PDF dependency first: python3 -m pip install --user reportlab (or sudo apt install python3-reportlab)")
    source=Path(args.dir)
    images=[];missing=[]
    for i in range(1,25):
        matches=[source/f"S{i:02d}.{ext}" for ext in ("png","jpg","jpeg")]
        existing=[p for p in matches if p.is_file()]
        if existing:images.append((i,existing[0]))
        else:missing.append(f"S{i:02d}")
    if missing:
        sys.exit("Missing genuine screenshots: "+", ".join(missing)+". Capture these in your own terminal; no PDF was generated.")
    dest=Path(f"{args.srn}.pdf")
    doc=SimpleDocTemplate(str(dest),pagesize=(595.28,841.89),leftMargin=36,rightMargin=36,topMargin=34,bottomMargin=34)
    styles=getSampleStyleSheet();story=[]
    for i,path in images:
        reader=ImageReader(str(path));w,h=reader.getSize()
        scale=min(523/w,690/h)
        story.extend([Paragraph(f"S{i:02d} — {CAPTIONS[i-1]}",styles["Normal"]),Spacer(1,10),Image(str(path),width=w*scale,height=h*scale)])
        if i<24:story.append(PageBreak())
    doc.build(story)
    print(f"Created {dest} from 24 actual user-provided screenshots")
    print("IMPORTANT: Manually verify every image contains your complete SRN in the terminal prompt.")
if __name__=="__main__":main()
