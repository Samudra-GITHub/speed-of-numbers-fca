# FCA Team Lab: The Speed of Numbers (ESP32)

A team investigation into numerical representation, hardware execution latency, and microarchitectural bottlenecks on the Espressif ESP32 (Xtensa LX6 @ 240 MHz).

---

## 📁 Repository Structure

```
FCA Assignment/
├── Speed_of_Numbers___Assignment.pdf   # Original laboratory specification
├── README.md                           # This guide
├── code/
│   └── speed_of_numbers/
│       └── speed_of_numbers.ino        # Full, production-ready Arduino sketch for all experiments
├── data/                               # Raw Serial Monitor output logs (as required by Section 7)
│   ├── exp1_stopwatch.txt
│   ├── exp2_integers.txt
│   ├── exp3_floats.txt
│   ├── exp4_bit_tricks.txt
│   ├── exp5_fixed_point.txt
│   ├── exp6_printing.txt
│   └── final_challenge.txt
└── latex/                              # Complete LaTeX report framework
    ├── main.tex                        # Master document with title, abstract & executive summary
    ├── preamble.tex                    # Professional typography, TikZ, Pgfplots, booktabs & listings
    └── sections/
        ├── 00_intro_toolbox.tex        # Platform setup, 3-member role matrix, Toolbox Q&A
        ├── 01_experiment1.tex          # Exp 1: Stopwatch calibration & loop baseline
        ├── 02_experiment2.tex          # Exp 2: Integer widths (8, 16, 32, 64-bit)
        ├── 03_experiment3.tex          # Exp 3: Float vs. Double & Hardware FPU vs. Soft-Float
        ├── 04_experiment4.tex          # Exp 4: Bit tricks vs. Arithmetic & Two's complement
        ├── 05_experiment5.tex          # Exp 5: Fixed-point Q16.16 representation & math
        ├── 06_experiment6.tex          # Exp 6: In-memory string formatting vs. UART 115200 latency
        ├── 07_final_challenge.tex      # Final Challenge: 10 kHz Sensor latency budget
        ├── 08_summary_comparison.tex   # Master comparison table & TikZ logarithmic chart
        ├── 09_biggest_surprise.tex     # The Biggest Surprise & deep architectural lessons
        ├── 10_team_reflection.tex      # Team reflection & LLM investigation partner analysis
        └── 11_appendix_code.tex        # Code reference and implementation snippets
```

---

## 👥 Team Roles (3-Member Rotation Strategy)

The assignment requires 4 roles (**Builder**, **Measurer**, **Skeptic**, **Scribe**) rotated so that everyone leads each role at least once.

- **You (User):** Primary Architect, Lead Builder (hardware upload & timing macros), Master Scribe (LaTeX authoring & synthesis).
- **Teammate 2:** Lead Measurer (serial data collection & statistical sorting), Builder for Exp 2 & 6, Scribe for Exp 4.
- **Teammate 3:** Lead Skeptic (compiler audit, `objdump` inspection, two's complement bug analysis), Builder for Exp 4, Scribe for Exp 3.

---

## ⚡ How to Run the Arduino Code (`speed_of_numbers.ino`)

1. Connect your **ESP32 Dev Board** to your computer via USB.
2. Open `code/speed_of_numbers/speed_of_numbers.ino` in **Arduino IDE**.
3. Under **Tools**:
   - **Board:** Select `ESP32 Dev Module` (or your specific board variant).
   - **CPU Frequency:** Select `240MHz (WiFi/BT)`.
   - **Upload Speed:** `921600` or `115200`.
   - **Port:** Select the COM port corresponding to your ESP32.
4. Click **Upload**. (If it says *Connecting...*, hold down the **BOOT** button on the board until uploading starts).
5. Open the **Serial Monitor** at **115200 baud** and set line ending to *Both NL & CR*.
6. Send single-character commands to execute tests:
   - `1` : Run Experiment 1 (Stopwatch Calibration)
   - `2` : Run Experiment 2 (Integer Widths: 8, 16, 32, 64-bit)
   - `3` : Run Experiment 3 (Float vs Double, FPU HW vs SW Emulation)
   - `4` : Run Experiment 4 (Bit Tricks vs Arithmetic, Two's Complement)
   - `5` : Run Experiment 5 (Fixed-Point Q16.16 Representation & Math)
   - `6` : Run Experiment 6 (Printing Cost & UART Transmission Latency)
   - `7` : Run Final Challenge (10 kHz Real-Time Sensor Pipeline)
   - `a` : Run **ALL** experiments sequentially to generate the full benchmark suite!

---

## 📄 How to Compile the LaTeX Report to PDF

The LaTeX project is 100% modular, self-contained, and uses native `pgfplots` / `tikz` (no external image files required).

### Option A: Overleaf (Zero Setup - Recommended)
1. Zip the entire `latex/` folder (or select `main.tex`, `preamble.tex`, and the `sections/` folder).
2. Go to [Overleaf](https://www.overleaf.com), click **New Project** $\to$ **Upload Project**, and upload the `.zip`.
3. Select **pdfLaTeX** or **XeLaTeX** as the compiler in Overleaf settings.
4. Click **Recompile**. Your report will compile instantly.

### Option B: Local Compilation via CLI / VS Code
If you have TeX Live or MiKTeX installed:
```bash
cd latex
pdflatex main.tex
pdflatex main.tex   # Run twice to resolve Table of Contents and cross-references
```
Or with `latexmk`:
```bash
latexmk -pdf main.tex
```

---

## 📊 Summary of Key Findings

> **Status: Pending hardware measurement.** No experiments have been run on physical ESP32 hardware yet. This section will be populated with real findings — backed by `data/*.txt` serial captures — once the team executes each experiment. Per project policy, no cycle counts, latency values, speedup ratios, or conclusions are recorded here until they are measured on the board.

1. **Integer Types:** *[Pending]*
2. **Float vs. Double:** *[Pending]*
3. **Bit Tricks & Two's Complement:** *[Pending]*
4. **Fixed-Point Comparison:** *[Pending]*
5. **The Hidden Cost of Printing:** *[Pending]*
6. **Final Challenge (10 kHz Budget):** *[Pending]*
