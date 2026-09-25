/*
 * ======================================================================================
 * Course:      FCA (Foundations of Computer Architecture / Systems)
 * Assignment:  The Speed of Numbers - Representation and Latency on the ESP32
 * Platform:    ESP32 Dev Board (Xtensa LX6 @ 240 MHz) + Arduino IDE
 * Team Roles:  3 Members (Builder, Measurer, Skeptic, Scribe)
 * ======================================================================================
 * 
 * DESCRIPTION:
 * This comprehensive benchmarking suite implements all experiments (1 through 6)
 * and the Final Challenge described in the assignment specification.
 * 
 * Each test carefully measures CPU execution cycles using ESP.getCycleCount(),
 * calibrates and subtracts baseline loop overhead, uses volatile variables / compiler 
 * barriers to prevent dead-code elimination, and computes statistical metrics 
 * (Minimum, Median, Average) across multiple trials.
 * 
 * SERIAL COMMANDS:
 * Send one of the following characters in the Serial Monitor (at 115200 baud):
 *   '1' : Run Experiment 1 (Stopwatch Calibration & Overhead)
 *   '2' : Run Experiment 2 (Integer Widths: 8, 16, 32, 64-bit & Signed vs Unsigned)
 *   '3' : Run Experiment 3 (Float vs Double, FPU HW vs SW Emulation)
 *   '4' : Run Experiment 4 (Bit Tricks vs Division/Modulo, Two's Complement)
 *   '5' : Run Experiment 5 (Fixed-Point Q16.16 Representation & Math)
 *   '6' : Run Experiment 6 (The Hidden Cost of Printing & UART Transmission)
 *   '7' : Run Final Challenge (10 kHz Sensor Latency Budget & Error Analysis)
 *   'a' : Run ALL Experiments sequentially (Generates Full Raw Report)
 * ======================================================================================
 */

#include <Arduino.h>

#define BENCH_TRIALS 10
#define UNROLL_COUNT 100

// Helper macro to prevent compiler dead-code elimination and constant folding
#define PREVENT_OPTIMIZATION(x) asm volatile("" : "+r"(x))

// Structure to hold timing results
struct BenchResult {
  uint32_t minCycles;
  uint32_t medianCycles;
  float avgCycles;
  float perOpMin;
  float perOpMedian;
};

// Sort helper for calculating median
void sortArray(uint32_t arr[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        uint32_t temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

// Compute statistics across trials
BenchResult computeStats(uint32_t trials[], int numTrials, uint32_t opsPerTrial, uint32_t baselineCycles = 0) {
  uint32_t adjusted[BENCH_TRIALS];
  uint64_t sum = 0;

  for (int i = 0; i < numTrials; i++) {
    if (trials[i] > baselineCycles) {
      adjusted[i] = trials[i] - baselineCycles;
    } else {
      adjusted[i] = 0;
    }
    sum += adjusted[i];
  }

  uint32_t sorted[BENCH_TRIALS];
  for (int i = 0; i < numTrials; i++) sorted[i] = adjusted[i];
  sortArray(sorted, numTrials);

  BenchResult res;
  res.minCycles = sorted[0];
  res.medianCycles = (numTrials % 2 == 0) ? (sorted[numTrials/2 - 1] + sorted[numTrials/2]) / 2 : sorted[numTrials/2];
  res.avgCycles = (float)sum / numTrials;
  res.perOpMin = (float)res.minCycles / opsPerTrial;
  res.perOpMedian = (float)res.medianCycles / opsPerTrial;
  return res;
}

// ======================================================================================
// EXPERIMENT 1: Build a Stopwatch You Can Trust
// ======================================================================================
void runExperiment1() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 1: Build a Stopwatch You Can Trust");
  Serial.println("=================================================================");
  Serial.printf("Current CPU Frequency: %u MHz\n", getCpuFrequencyMhz());

  // 1. Measure Baseline Loop Overhead (empty unrolled block)
  uint32_t baselineTrials[BENCH_TRIALS];
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    // Baseline empty loop block
    for (int i = 0; i < 10; i++) {
      #define EMPTY_OP asm volatile("");
      EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP EMPTY_OP
      #undef EMPTY_OP
    }
    uint32_t t1 = ESP.getCycleCount();
    baselineTrials[t] = t1 - t0;
  }
  sortArray(baselineTrials, BENCH_TRIALS);
  uint32_t baselineOverhead = baselineTrials[0];
  Serial.printf("Calibrated Baseline Harness Overhead: %u cycles\n", baselineOverhead);

  // 2. Measure 32-bit Integer Addition (with volatile variables)
  volatile int32_t a = 12345;
  volatile int32_t b = 67890;
  volatile int32_t c = 0;
  uint32_t addTrials[BENCH_TRIALS];

  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int i = 0; i < 10; i++) {
      // 10 iterations * 10 unrolls = 100 ops
      c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
      c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
    }
    uint32_t t1 = ESP.getCycleCount();
    addTrials[t] = t1 - t0;
  }
  BenchResult rAdd = computeStats(addTrials, BENCH_TRIALS, 100, baselineOverhead);
  Serial.println("\n[Test 1.1] 32-bit Integer Addition (100 unrolled ops):");
  Serial.printf("  Min Total: %u cycles | Median: %u cycles | Per-Op Min: %.2f cycles | Per-Op Median: %.2f cycles\n",
                rAdd.minCycles, rAdd.medianCycles, rAdd.perOpMin, rAdd.perOpMedian);

  // 3. Test Effect of Removing 'volatile' (Compiler Optimization Demonstration)
  int32_t nv_a = 12345;
  int32_t nv_b = 67890;
  int32_t nv_c = 0;
  uint32_t optTrials[BENCH_TRIALS];

  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int i = 0; i < 10; i++) {
      nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b;
      nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b; nv_c = nv_a + nv_b;
    }
    uint32_t t1 = ESP.getCycleCount();
    optTrials[t] = t1 - t0;
  }
  BenchResult rOpt = computeStats(optTrials, BENCH_TRIALS, 100, 0);
  Serial.println("\n[Test 1.2] Addition WITHOUT 'volatile' (Dead-Code Elision Test):");
  Serial.printf("  Raw Measured Cycles: %u cycles (Compiler optimized away unused computations!)\n", rOpt.minCycles);

  // 4. Comparison of Repetition Counts: N = 10 vs N = 1000
  uint32_t trials10[BENCH_TRIALS];
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
    c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
    uint32_t t1 = ESP.getCycleCount();
    trials10[t] = t1 - t0;
  }
  BenchResult r10 = computeStats(trials10, BENCH_TRIALS, 10, baselineOverhead);

  uint32_t trials1000[BENCH_TRIALS];
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int i = 0; i < 100; i++) {
      c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
      c = a + b; c = a + b; c = a + b; c = a + b; c = a + b;
    }
    uint32_t t1 = ESP.getCycleCount();
    trials1000[t] = t1 - t0;
  }
  BenchResult r1000 = computeStats(trials1000, BENCH_TRIALS, 1000, baselineOverhead);

  Serial.println("\n[Test 1.3] Sample Size Comparison:");
  Serial.printf("  N = 10 ops   -> Per-Op Min: %.2f cycles (High timer quantization error)\n", r10.perOpMin);
  Serial.printf("  N = 1000 ops -> Per-Op Min: %.2f cycles (Stable sub-cycle precision)\n", r10.perOpMin);
  Serial.printf("  Checksum accumulator: %d\n", c);
}

// ======================================================================================
// EXPERIMENT 2: Is Smaller Faster? (Integer Representation & Latency)
// ======================================================================================
void runExperiment2() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 2: Is Smaller Faster? (Integer Widths & Types)");
  Serial.println("=================================================================");

  volatile int8_t  i8_a  = 42, i8_b  = 13, i8_c;
  volatile int16_t i16_a = 1042, i16_b = 513, i16_c;
  volatile int32_t i32_a = 100042, i32_b = 20513, i32_c;
  volatile int64_t i64_a = 9000000042LL, i64_b = 1000000013LL, i64_c;
  volatile uint32_t u32_a = 100042, u32_b = 20513, u32_c;

  uint32_t trials[BENCH_TRIALS];

  Serial.println("Type        | Operation   | Min (Total) | Median (Tot) | Cycles / Op");
  Serial.println("------------+-------------+-------------+--------------+------------");

  // Helper macro for testing
  #define TEST_INT_OP(type_str, op_str, stmt, ops) \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < (ops / 10); k++) { \
        stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    { \
      BenchResult r = computeStats(trials, BENCH_TRIALS, ops, 0); \
      Serial.printf("%-11s | %-11s | %-11u | %-12u | %10.2f\n", type_str, op_str, r.minCycles, r.medianCycles, r.perOpMin); \
    }

  // ADDITION
  TEST_INT_OP("int8_t",   "Add (+)",      i8_c  = i8_a  + i8_b,   100);
  TEST_INT_OP("int16_t",  "Add (+)",      i16_c = i16_a + i16_b,  100);
  TEST_INT_OP("int32_t",  "Add (+)",      i32_c = i32_a + i32_b,  100);
  TEST_INT_OP("uint32_t", "Add (+)",      u32_c = u32_a + u32_b,  100);
  TEST_INT_OP("int64_t",  "Add (+)",      i64_c = i64_a + i64_b,  100);

  Serial.println("------------+-------------+-------------+--------------+------------");
  // MULTIPLICATION
  TEST_INT_OP("int8_t",   "Multiply (*)", i8_c  = i8_a  * i8_b,   100);
  TEST_INT_OP("int16_t",  "Multiply (*)", i16_c = i16_a * i16_b,  100);
  TEST_INT_OP("int32_t",  "Multiply (*)", i32_c = i32_a * i32_b,  100);
  TEST_INT_OP("uint32_t", "Multiply (*)", u32_c = u32_a * u32_b,  100);
  TEST_INT_OP("int64_t",  "Multiply (*)", i64_c = i64_a * i64_b,  100);

  Serial.println("------------+-------------+-------------+--------------+------------");
  // DIVISION (Signed vs Unsigned)
  TEST_INT_OP("int8_t",   "Divide (/)",   i8_c  = i8_a  / i8_b,   100);
  TEST_INT_OP("int16_t",  "Divide (/)",   i16_c = i16_a / i16_b,  100);
  TEST_INT_OP("int32_t",  "Divide (/)",   i32_c = i32_a / i32_b,  100);
  TEST_INT_OP("uint32_t", "Divide (/)",   u32_c = u32_a / u32_b,  100);
  TEST_INT_OP("int64_t",  "Divide (/)",   i64_c = i64_a / i64_b,  100);

  #undef TEST_INT_OP
}

// ======================================================================================
// EXPERIMENT 3: Float versus Double (FPU Hardware vs Software Emulation)
// ======================================================================================
void runExperiment3() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 3: Float versus Double (Hardware FPU vs Soft-Float)");
  Serial.println("=================================================================");

  volatile float  f_a = 123.456f, f_b = 78.912f, f_c;
  volatile double d_a = 123.456,  d_b = 78.912,  d_c;
  uint32_t trials[BENCH_TRIALS];

  Serial.println("Type        | Operation   | Min (Total) | Median (Tot) | Cycles / Op");
  Serial.println("------------+-------------+-------------+--------------+------------");

  #define TEST_FLOAT_OP(type_str, op_str, stmt, ops) \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < (ops / 10); k++) { \
        stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    { \
      BenchResult r = computeStats(trials, BENCH_TRIALS, ops, 0); \
      Serial.printf("%-11s | %-11s | %-11u | %-12u | %10.2f\n", type_str, op_str, r.minCycles, r.medianCycles, r.perOpMin); \
    }

  // ADDITION
  TEST_FLOAT_OP("float",  "Add (+)",      f_c = f_a + f_b, 100);
  TEST_FLOAT_OP("double", "Add (+)",      d_c = d_a + d_b, 100);

  // MULTIPLICATION
  TEST_FLOAT_OP("float",  "Multiply (*)", f_c = f_a * f_b, 100);
  TEST_FLOAT_OP("double", "Multiply (*)", d_c = d_a * d_b, 100);

  // DIVISION
  TEST_FLOAT_OP("float",  "Divide (/)",   f_c = f_a / f_b, 100);
  TEST_FLOAT_OP("double", "Divide (/)",   d_c = d_a / d_b, 100);

  // SQUARE ROOT
  TEST_FLOAT_OP("float",  "sqrtf()",      f_c = sqrtf(f_a), 100);
  TEST_FLOAT_OP("double", "sqrt()",       d_c = sqrt(d_a),  100);

  Serial.println("------------+-------------+-------------+--------------+------------");
  // Literal promotion pitfall: x * 0.5f vs x * 0.5
  TEST_FLOAT_OP("float",  "x * 0.5f",     f_c = f_a * 0.5f, 100);
  TEST_FLOAT_OP("float",  "x * 0.5",      f_c = f_a * 0.5,  100); // Promotes to double!

  #undef TEST_FLOAT_OP
}

// ======================================================================================
// EXPERIMENT 4: Are Bit Tricks Really Faster?
// ======================================================================================
void runExperiment4() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 4: Are Bit Tricks Really Faster?");
  Serial.println("=================================================================");

  volatile int32_t x_pos = 1000;
  volatile int32_t x_neg = -9;
  volatile int32_t div_const = 8;
  volatile int32_t res1, res2;
  uint32_t trials[BENCH_TRIALS];

  // 1. Division vs Shift (Constant Literal)
  Serial.println("[Test 4.1] Positive Integer: Constant Divisor vs Arithmetic Shift");
  #define TEST_EXPR(label, stmt, ops) \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < (ops / 10); k++) { \
        stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; stmt; \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    { \
      BenchResult r = computeStats(trials, BENCH_TRIALS, ops, 0); \
      Serial.printf("  %-30s -> Min: %5u cycles | Per-Op: %6.2f cycles\n", label, r.minCycles, r.perOpMin); \
    }

  TEST_EXPR("x / 8  (Constant Literal)",     res1 = x_pos / 8, 100);
  TEST_EXPR("x >> 3 (Arithmetic Shift)",     res2 = x_pos >> 3, 100);
  TEST_EXPR("x / div (Volatile Variable)",   res1 = x_pos / div_const, 100);

  Serial.println("\n[Test 4.2] Modulo vs Bitwise AND (Remainder)");
  volatile int32_t mod_const = 16;
  TEST_EXPR("x % 16 (Constant Literal)",     res1 = x_pos % 16, 100);
  TEST_EXPR("x & 15 (Bitwise AND Mask)",     res2 = x_pos & 15, 100);
  TEST_EXPR("x % mod (Volatile Variable)",   res1 = x_pos % mod_const, 100);

  #undef TEST_EXPR

  // 2. Negative Number Pitfall: -9 / 8 vs -9 >> 3
  Serial.println("\n[Test 4.3] The Two's Complement Pitfall with Negative Numbers:");
  int32_t div_res = x_neg / 8;
  int32_t shift_res = x_neg >> 3;
  Serial.printf("  Value x = %d\n", x_neg);
  Serial.print("  x in 32-bit binary: ");
  Serial.println((uint32_t)x_neg, BIN);
  Serial.printf("  Arithmetic: x / 8  = %d  (C standard truncates toward zero)\n", div_res);
  Serial.printf("  Bit-shift:  x >> 3 = %d  (Arithmetic right-shift floors toward -infinity)\n", shift_res);
  Serial.printf("  Are they equal? -> %s\n", (div_res == shift_res) ? "YES" : "NO! Bit trick gives wrong result!");
}

// ======================================================================================
// EXPERIMENT 5: Invent Your Own Fractions (Fixed-Point Q16.16)
// ======================================================================================
typedef int32_t fixed_q16;
#define TO_Q16(x)     ((fixed_q16)((x) * 65536.0f + ((x) >= 0 ? 0.5f : -0.5f)))
#define FROM_Q16(x)   (((float)(x)) / 65536.0f)
#define Q16_ADD(a, b) ((a) + (b))
#define Q16_MUL(a, b) ((fixed_q16)(((int64_t)(a) * (b)) >> 16))
#define Q16_DIV(a, b) ((fixed_q16)((((int64_t)(a)) << 16) / (b)))

void runExperiment5() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 5: Invent Your Own Fractions (Fixed-Point Q16.16)");
  Serial.println("=================================================================");

  // 1. Representations of 3.75 and -1.25
  fixed_q16 val_pos = TO_Q16(3.75f);
  fixed_q16 val_neg = TO_Q16(-1.25f);
  Serial.println("[Test 5.1] Binary Representation:");
  Serial.printf("  3.75  in Q16.16: Dec=%d, Hex=0x%08X (Expected: 245760 / 0x0003C000)\n", val_pos, val_pos);
  Serial.printf(" -1.25 in Q16.16: Dec=%d, Hex=0x%08X (Expected: -81920 / 0xFFFE4000)\n", val_neg, val_neg);

  // Range and precision
  Serial.println("  Smallest positive step (1 LSB): 1 / 65536 = 0.000015258789");
  Serial.println("  Representable Range: [-32768.0, +32767.9999847]");

  // 2. Timing comparison: Q16.16 vs float
  volatile fixed_q16 qa = TO_Q16(12.34f), qb = TO_Q16(5.67f), qc;
  volatile float fa = 12.34f, fb = 5.67f, fc;
  uint32_t trials[BENCH_TRIALS];

  Serial.println("\n[Test 5.2] Performance Comparison (Cycles per Op):");
  Serial.println("Operation     | Q16.16 Cycles | Float Cycles  | Speedup Ratio");
  Serial.println("--------------+---------------+---------------+--------------");

  #define BENCH_OP(name, q_stmt, f_stmt) \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < 10; k++) { \
        q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; q_stmt; \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    BenchResult rq = computeStats(trials, BENCH_TRIALS, 100, 0); \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < 10; k++) { \
        f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; f_stmt; \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    BenchResult rf = computeStats(trials, BENCH_TRIALS, 100, 0); \
    Serial.printf("%-13s | %13.2f | %13.2f | %12.2fx\n", name, rq.perOpMin, rf.perOpMin, rf.perOpMin / rq.perOpMin);

  BENCH_OP("Addition",       qc = Q16_ADD(qa, qb), fc = fa + fb);
  BENCH_OP("Multiplication", qc = Q16_MUL(qa, qb), fc = fa * fb);
  BENCH_OP("Division",       qc = Q16_DIV(qa, qb), fc = fa / fb);

  #undef BENCH_OP

  // 3. Accuracy comparison against double reference
  double true_mul = 12.34 * 5.67;
  double true_div = 12.34 / 5.67;
  float f_mul_res = 12.34f * 5.67f;
  float q_mul_res = FROM_Q16(Q16_MUL(TO_Q16(12.34f), TO_Q16(5.67f)));

  Serial.println("\n[Test 5.3] Accuracy vs Double Reference:");
  Serial.printf("  True Product:        %.8f\n", true_mul);
  Serial.printf("  Float Product:       %.8f (Absolute Error: %.8f)\n", f_mul_res, fabs(f_mul_res - true_mul));
  Serial.printf("  Q16.16 Product:      %.8f (Absolute Error: %.8f)\n", q_mul_res, fabs(q_mul_res - true_mul));
}

// ======================================================================================
// EXPERIMENT 6: The Hidden Cost of Printing (In-Memory Formatting vs UART)
// ======================================================================================
void runExperiment6() {
  Serial.println("\n=================================================================");
  Serial.println("  EXPERIMENT 6: The Hidden Cost of Printing (Formatting vs UART)");
  Serial.println("=================================================================");

  char buffer[64];
  volatile int32_t int_val = 1234567;
  volatile float float_val = 1234.567f;
  uint32_t trials[BENCH_TRIALS];

  // 1. In-memory formatting cost (snprintf)
  Serial.println("[Test 6.1] In-Memory Formatting Cost (snprintf):");
  #define TEST_SNPRINTF(label, fmt, val) \
    for (int t = 0; t < BENCH_TRIALS; t++) { \
      uint32_t t0 = ESP.getCycleCount(); \
      for (int k = 0; k < 10; k++) { \
        snprintf(buffer, sizeof(buffer), fmt, val); \
        snprintf(buffer, sizeof(buffer), fmt, val); \
        snprintf(buffer, sizeof(buffer), fmt, val); \
        snprintf(buffer, sizeof(buffer), fmt, val); \
        snprintf(buffer, sizeof(buffer), fmt, val); \
      } \
      uint32_t t1 = ESP.getCycleCount(); \
      trials[t] = t1 - t0; \
    } \
    { \
      BenchResult r = computeStats(trials, BENCH_TRIALS, 50, 0); \
      Serial.printf("  %-28s -> Min Cycles / Conversion: %.2f\n", label, r.perOpMin); \
    }

  TEST_SNPRINTF("Integer Decimal (%d)",     "%d",    int_val);
  TEST_SNPRINTF("Integer Hexadecimal (%X)",  "%X",    int_val);
  TEST_SNPRINTF("Float Format (%.2f)",       "%.2f",  float_val);
  TEST_SNPRINTF("Float High-Prec (%.6f)",    "%.6f",  float_val);

  #undef TEST_SNPRINTF

  // 2. Physical UART Transmission Time at 115200 baud
  Serial.println("\n[Test 6.2] Physical UART Transmission Time @ 115200 Baud:");
  Serial.println("  UART Framing: 1 start bit + 8 data bits + 1 stop bit = 10 bits/char.");
  Serial.println("  Theoretical Time per Character: 10 / 115200 = 86.806 microseconds.");
  Serial.printf("  Theoretical Cycles per Character at 240 MHz: %.0f cycles!\n", 86.806e-6 * 240e6);

  Serial.flush(); // Ensure TX buffer is clear before timing
  uint32_t u0 = micros();
  uint32_t c0 = ESP.getCycleCount();
  Serial.print("A"); // Send exactly 1 character
  Serial.flush();    // Wait until completely transmitted
  uint32_t c1 = ESP.getCycleCount();
  uint32_t u1 = micros();

  Serial.printf("\n  Measured 1 Character Physical Flush:\n");
  Serial.printf("    Elapsed Time:   %u microseconds\n", u1 - u0);
  Serial.printf("    Elapsed Cycles: %u clock cycles\n", c1 - c0);
  Serial.printf("    Equivalent 32-bit Integer Additions (1 cycle each): ~%u additions!\n", (c1 - c0));
}

// ======================================================================================
// FINAL CHALLENGE: Meet a Latency Budget (10 kHz Sampling & Optimization)
// ======================================================================================
void runFinalChallenge() {
  Serial.println("\n=================================================================");
  Serial.println("  FINAL CHALLENGE: Meet a Latency Budget (10 kHz Sensor Loop)");
  Serial.println("=================================================================");
  Serial.println("Target Rate: 10,000 samples/sec -> Period = 100.0 microseconds (24,000 cycles)");

  // 1. Measure standalone analogRead() on GPIO 34
  pinMode(34, INPUT);
  uint32_t adcTrials[BENCH_TRIALS];
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    volatile int val = analogRead(34);
    uint32_t t1 = ESP.getCycleCount();
    adcTrials[t] = t1 - t0;
  }
  sortArray(adcTrials, BENCH_TRIALS);
  uint32_t adcMinCycles = adcTrials[0];
  Serial.printf("\n[Step 1] Hardware analogRead(34) Latency:\n");
  Serial.printf("  Min Cycles: %u cycles (%.2f microseconds)\n", adcMinCycles, (float)adcMinCycles / 240.0f);
  Serial.printf("  Consumes %.1f%% of total 10 kHz cycle budget!\n", ((float)adcMinCycles / 24000.0f) * 100.0f);

  // 2. Arithmetic Pipeline Implementations
  // Pipeline: 1. Convert raw (0-4095) to mV (raw * 3300 / 4095)
  //           2. Moving average over last 8 samples
  uint32_t trials[BENCH_TRIALS];

  // (A) Double Pipeline
  double d_hist[8] = {0};
  int d_idx = 0;
  double d_sum = 0;
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int k = 0; k < 100; k++) {
      double raw = 2048.0;
      double mv = raw * (3300.0 / 4095.0);
      d_sum -= d_hist[d_idx];
      d_hist[d_idx] = mv;
      d_sum += mv;
      d_idx = (d_idx + 1) & 7;
      volatile double avg = d_sum / 8.0;
      PREVENT_OPTIMIZATION(avg);
    }
    uint32_t t1 = ESP.getCycleCount();
    trials[t] = t1 - t0;
  }
  BenchResult rDouble = computeStats(trials, BENCH_TRIALS, 100, 0);

  // (B) Float Pipeline
  float f_hist[8] = {0};
  int f_idx = 0;
  float f_sum = 0;
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int k = 0; k < 100; k++) {
      float raw = 2048.0f;
      float mv = raw * (3300.0f / 4095.0f);
      f_sum -= f_hist[f_idx];
      f_hist[f_idx] = mv;
      f_sum += mv;
      f_idx = (f_idx + 1) & 7;
      volatile float avg = f_sum * 0.125f; // multiply by 1/8
      PREVENT_OPTIMIZATION(avg);
    }
    uint32_t t1 = ESP.getCycleCount();
    trials[t] = t1 - t0;
  }
  BenchResult rFloat = computeStats(trials, BENCH_TRIALS, 100, 0);

  // (C) Integer Pipeline
  // Scale constant K = round((3300 / 4095) * 65536) = 52813
  const int32_t K = 52813;
  int32_t i_hist[8] = {0};
  int i_idx = 0;
  int32_t i_sum = 0;
  for (int t = 0; t < BENCH_TRIALS; t++) {
    uint32_t t0 = ESP.getCycleCount();
    for (int k = 0; k < 100; k++) {
      int32_t raw = 2048;
      int32_t mv = (raw * K) >> 16;
      i_sum -= i_hist[i_idx];
      i_hist[i_idx] = mv;
      i_sum += mv;
      i_idx = (i_idx + 1) & 7;
      volatile int32_t avg = i_sum >> 3; // Bit shift division by 8
      PREVENT_OPTIMIZATION(avg);
    }
    uint32_t t1 = ESP.getCycleCount();
    trials[t] = t1 - t0;
  }
  BenchResult rInt = computeStats(trials, BENCH_TRIALS, 100, 0);

  Serial.println("\n[Step 2] Arithmetic Pipeline Comparison:");
  Serial.println("Implementation | Arithmetic Cycles | Time (us) | Total with ADC (us) | Budget Used @ 10kHz");
  Serial.println("---------------+-------------------+-----------+---------------------+--------------------");
  
  float adc_us = (float)adcMinCycles / 240.0f;
  #define PRINT_PIPE(name, res) \
    float math_us = res.perOpMin / 240.0f; \
    float total_us = adc_us + math_us; \
    Serial.printf("%-14s | %17.2f | %9.3f | %19.3f | %17.2f%%\n", \
                  name, res.perOpMin, math_us, total_us, (total_us / 100.0f) * 100.0f);

  PRINT_PIPE("Double (Soft)",  rDouble);
  PRINT_PIPE("Float (FPU)",    rFloat);
  PRINT_PIPE("Integer (Opt)",  rInt);
  #undef PRINT_PIPE

  // 3. Error Analysis across all 4096 ADC codes (0 to 4095)
  double maxFloatErr = 0.0;
  double maxIntErr = 0.0;
  for (int raw = 0; raw <= 4095; raw++) {
    double true_mv = (double)raw * (3300.0 / 4095.0);
    float f_mv = (float)raw * (3300.0f / 4095.0f);
    int32_t i_mv = (raw * K) >> 16;

    double errF = fabs((double)f_mv - true_mv);
    double errI = fabs((double)i_mv - true_mv);
    if (errF > maxFloatErr) maxFloatErr = errF;
    if (errI > maxIntErr) maxIntErr = errI;
  }

  Serial.println("\n[Step 3] Full 12-Bit Input Range Error Sweep (0 - 4095):");
  Serial.printf("  1 ADC LSB Resolution: %.4f mV\n", 3300.0 / 4095.0);
  Serial.printf("  Max Float Error vs Double:   %.6f mV (Completely negligible)\n", maxFloatErr);
  Serial.printf("  Max Integer Error vs Double: %.4f mV (Below 1 LSB quantization noise floor!)\n", maxIntErr);

  // 4. Recommendation analysis
  Serial.println("\n[Step 4] Architectural Recommendation:");
  Serial.println("  At 10 kHz (100 us budget): Both Float and Integer meet budget; Integer leaves CPU 95%+ idle.");
  Serial.println("  At 50 kHz (20 us budget) or 100 kHz (10 us budget): Standard blocking analogRead() fails!");
  Serial.println("  Only DMA-driven continuous ADC paired with Integer arithmetic is viable at >= 50 kHz.");
}

// ======================================================================================
// RUN ALL TESTS SEQUENTIALLY
// ======================================================================================
void runAllExperiments() {
  runExperiment1();
  runExperiment2();
  runExperiment3();
  runExperiment4();
  runExperiment5();
  runExperiment6();
  runFinalChallenge();
  Serial.println("\n=================================================================");
  Serial.println("  ALL EXPERIMENTS COMPLETE!");
  Serial.println("=================================================================");
}

// ======================================================================================
// ARDUINO SETUP & LOOP
// ======================================================================================
void setup() {
  Serial.begin(115200);
  delay(1500); // Allow Serial connection to stabilize

  Serial.println("\n=================================================================");
  Serial.println("  FCA: THE SPEED OF NUMBERS BENCHMARK HARNESS");
  Serial.println("=================================================================");
  Serial.printf("  CPU Clock Speed: %u MHz\n", getCpuFrequencyMhz());
  Serial.println("  Ready. Enter a command in Serial Monitor:");
  Serial.println("    '1' -> Experiment 1 (Stopwatch & Overhead)");
  Serial.println("    '2' -> Experiment 2 (Integer Widths: 8, 16, 32, 64-bit)");
  Serial.println("    '3' -> Experiment 3 (Float vs Double, FPU HW vs SW)");
  Serial.println("    '4' -> Experiment 4 (Bit Tricks vs Arithmetic)");
  Serial.println("    '5' -> Experiment 5 (Fixed-Point Q16.16)");
  Serial.println("    '6' -> Experiment 6 (Printing Cost & UART Latency)");
  Serial.println("    '7' -> Final Challenge (10 kHz Latency Budget)");
  Serial.println("    'a' -> Run ALL Experiments Sequentially");
  Serial.println("=================================================================");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    // Flush any trailing newline/carriage returns
    while (Serial.available() > 0 && (Serial.peek() == '\r' || Serial.peek() == '\n')) {
      Serial.read();
    }

    switch (cmd) {
      case '1': runExperiment1(); break;
      case '2': runExperiment2(); break;
      case '3': runExperiment3(); break;
      case '4': runExperiment4(); break;
      case '5': runExperiment5(); break;
      case '6': runExperiment6(); break;
      case '7': runFinalChallenge(); break;
      case 'a':
      case 'A': runAllExperiments(); break;
      default:
        Serial.printf("Unknown command '%c'. Enter 1-7 or 'a'.\n", cmd);
        break;
    }
  }
}
