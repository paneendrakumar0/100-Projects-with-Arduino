/*
 * 100 Projects with Arduino - Day 57
 * Project: FFT Audio Spectrum Analyzer (Cooley-Tukey Radix-2 DIT FFT from scratch)
 *
 * DESCRIPTION:
 * This project implements a real-time Fast Fourier Transform (FFT) audio spectrum analyzer.
 * It samples an analog microphone sensor at a precise, deterministic sample frequency (4000 Hz)
 * and processes the waveforms using an in-place decimation-in-time (DIT) Cooley-Tukey Radix-2 FFT
 * algorithm written completely from scratch (no arduinoFFT library).
 *
 * DIGITAL SIGNAL PROCESSING (DSP) MATH:
 * - Sampling Parameters:
 *   - Sample count (N) = 64 points.
 *   - Sampling period (dt) = 250us -> Sample Rate (Fs) = 4000 Hz.
 *   - Nyquist Frequency (highest detectable frequency) = Fs / 2 = 2000 Hz.
 *   - Frequency Bin Resolution (df) = Fs / N = 4000 / 64 = 62.5 Hz.
 * - Cooley-Tukey Algorithm:
 *   1. Bit-Reversal Permutation: Reorders the input data index bits to align values for butterfly
 * math.
 *   2. Butterfly Computations: Combines pairs of points recursively using complex twiddle factors
 *      W_N^k = cos(theta) - j * sin(theta), where theta = -2 * PI * k / len.
 *   3. Magnitude Extraction: Computes the absolute length of the resulting complex vectors:
 *      Magnitude = sqrt(Real^2 + Imag^2)
 *
 * CONSOLE VISUALIZATION:
 * Outputs the 32 positive frequency bins (0 to 2000 Hz) as a vertical ASCII bar chart directly
 * in the Serial Monitor, providing a dynamic frequency visualizer without external display
 * overhead.
 *
 * WIRING:
 * - Analog Microphone Module (MAX4466 or KY-037/038) -> Arduino Uno
 *   - VCC  -> 5V (or 3.3V for cleaner analog reference)
 *   - GND  -> GND
 *   - OUT  -> Pin A0 (Analog Input)
 */

// --- DSP CONFIGURATION ---
const int N_SAMPLES = 64;                      // Must be a power of 2 for Radix-2 Cooley-Tukey FFT
const unsigned long SAMPLING_PERIOD_US = 250;  // 250us interval = 4000 Hz sample rate
const float RESOLUTION_DF = 62.5;              // Fs / N = 4000 / 64 = 62.5 Hz per bin

float vReal[N_SAMPLES];
float vImag[N_SAMPLES];

void setup() {
  Serial.begin(115200);  // High-speed baud rate for rapid visualizer printing

  pinMode(A0, INPUT);

  Serial.println(F("[DSP] FFT Audio Spectrum Analyzer active. Ready to sample A0..."));
  delay(1000);
}

void loop() {
  // Step 1: Record 64 analog audio samples at a deterministic 4000 Hz rate
  for (int i = 0; i < N_SAMPLES; i++) {
    unsigned long tStart = micros();

    // Read analog voltage and subtract DC offset bias (~512 for a 2.5V biased mic signal)
    int rawValue = analogRead(A0);
    vReal[i] = (float)rawValue - 512.0;
    vImag[i] = 0.0;  // Imaginary component is initialized to 0

    // Precise software timing sync lock
    while (micros() - tStart < SAMPLING_PERIOD_US) {
      // Busy wait to maintain deterministic sample rate
    }
  }

  // Step 2: Run Radix-2 Decimation-In-Time Cooley-Tukey FFT
  executeFFT(vReal, vImag, N_SAMPLES);

  // Step 3: Compute magnitudes and visualize spectrum in console
  renderASCIIBars();

  delay(150);  // Frame refresh delay
}

// --- COOLEY-TUKEY RADIX-2 FFT ENGINE ---

void executeFFT(float* real, float* imag, uint16_t samples) {
  // --- 1. Bit-Reversal Permutation ---
  // Re-orders the input array indices to prepare for DIT butterflies
  uint16_t j = 0;
  for (uint16_t i = 0; i < samples; i++) {
    if (i < j) {
      // Swap real components
      float tempReal = real[i];
      real[i] = real[j];
      real[j] = tempReal;
      // Swap imaginary components
      float tempImag = imag[i];
      imag[i] = imag[j];
      imag[j] = tempImag;
    }

    uint16_t m = samples >> 1;
    while (j >= m && m >= 2) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }

  // --- 2. Cooley-Tukey Butterfly Loop ---
  // Outer loop: doubles the sub-FFT block length at each stage (2, 4, 8 ... N)
  for (uint16_t len = 2; len <= samples; len <<= 1) {
    float theta = -2.0 * PI / len;
    float wpr = cos(theta);  // Cosine real factor
    float wpi = sin(theta);  // Sine imaginary factor

    // Middle loop: jumps from block to block
    for (uint16_t i = 0; i < samples; i += len) {
      float wr = 1.0;
      float wi = 0.0;
      uint16_t halfLen = len >> 1;

      // Inner loop: performs butterfly calculation within each block
      for (uint16_t k = 0; k < halfLen; k++) {
        uint16_t u = i + k;
        uint16_t v = u + halfLen;

        // Complex multiplication: ComplexTemp = v[v] * w
        float tr = real[v] * wr - imag[v] * wi;
        float ti = real[v] * wi + imag[v] * wr;

        // Butterfly equations
        real[v] = real[u] - tr;
        imag[v] = imag[u] - ti;
        real[u] += tr;
        imag[u] += ti;

        // Trigonometric recurrence formula to update twiddle factor (w = w * wp)
        float temp = wr;
        wr = wr * wpr - wi * wpi;
        wi = wi * wpr + temp * wpi;
      }
    }
  }
}

// --- CONSOLE BAR CHART DISPLAY ---

void renderASCIIBars() {
  // Print line feeds to flush the console window for a clean frame updates
  for (int i = 0; i < 15; i++) Serial.println();

  Serial.println(F("================= FFT AUDIO SPECTRUM (0 - 2000 Hz) ================="));

  // Since the output spectrum is symmetrical, we only plot the first N/2 bins (0 to Nyquist)
  for (int bin = 1; bin < N_SAMPLES / 2; bin++) {
    // Extract magnitude: Mag = sqrt(Real^2 + Imag^2)
    float mag = sqrt(vReal[bin] * vReal[bin] + vImag[bin] * vImag[bin]);

    // Scale magnitude for display (Uno analog input values mapped to visual console scale)
    int barWidth = (int)(mag / 18.0);
    if (barWidth > 40) barWidth = 40;  // Clamping maximum width

    // Calculate bin center frequency
    float freq = (float)bin * RESOLUTION_DF;

    // Print frequency label
    Serial.print(freq, 0);
    Serial.print(F(" Hz\t["));
    if (freq < 100.0) Serial.print(F(" "));  // Column alignment spacing
    if (freq < 1000.0) Serial.print(F(" "));

    // Draw the bar using asterisks
    for (int i = 0; i < barWidth; i++) {
      Serial.print(F("*"));
    }
    Serial.println();
  }
  Serial.println(F("===================================================================="));
}
