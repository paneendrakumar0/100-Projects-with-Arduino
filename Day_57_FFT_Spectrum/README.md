# Day 57: FFT Audio Spectrum Analyzer

Welcome to Day 57 of the 100-Day Arduino Masterclass! Today, we enter the domain of **digital signal processing (DSP)** by building a real-time **Audio Spectrum Analyzer**. We will write a raw **in-place Decimation-in-Time (DIT) Cooley-Tukey Radix-2 Fast Fourier Transform (FFT) algorithm** from scratch (no helper DSP libraries), parse raw analog sound waves into frequency bins, and render an active ASCII frequency spectrum bar chart directly to the Serial Monitor.

---


## 📸 Component Visuals

<p align="center">
  <img src="../assets/images/components/Arduino_Uno.jpg" alt="Arduino Uno" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Breadboard.jpg" alt="Breadboard" width="200" style="margin:10px;" />
  <img src="../assets/images/components/Jumper_Wires.jpg" alt="Jumper Wires" width="200" style="margin:10px;" />
  <img src="../assets/images/components/LED.jpg" alt="LED" width="200" style="margin:10px;" />
</p>

## 🎯 The "Why" and "What"

Microcontrollers read raw analog signals as a time-series voltage wave.
* **The Problem:** Looking at a raw sound wave only tells you the overall volume at any instant. It cannot tell you what notes are being played or identify separate sound pitches. Converting a signal from the **Time Domain** to the **Frequency Domain** mathematically (Fourier Transform) normally requires massive floating-point arithmetic processors.
* **The Solution:** We implement the highly optimized **Cooley-Tukey Radix-2 FFT**. By choosing a sample count of $N=64$ (a power of 2) and sampling at a precise, locked rate of $4000\,\text{Hz}$ using software timer gates, we compute the full frequency spectrum on the Arduino Uno in under **$7\,\text{ms}$**, displaying frequencies from $0$ to $2000\,\text{Hz}$.

---

## 🔬 Physics & Mathematics

### 1. Nyquist-Shannon Sampling Theorem
To digitize an analog signal without losing information or creating false phantom frequencies (**aliasing**), the sampling frequency ($f_s$) must be at least twice the highest frequency component ($f_{\text{max}}$) present in the signal:
$$f_s > 2 \cdot f_{\text{max}}$$

With $f_s = 4000\,\text{Hz}$ ($dt = 250\,\mu\text{s}$), our detectable range (**Nyquist Frequency**) limit is:
$$f_{\text{Nyquist}} = \frac{f_s}{2} = 2000\,\text{Hz}$$

---

### 2. Frequency Bin Resolution
The FFT divides the frequency spectrum into $N/2$ positive bins. The frequency width of each individual bin ($\Delta f$) is determined by:
$$\Delta f = \frac{f_s}{N} = \frac{4000\,\text{Hz}}{64} = 62.5\,\text{Hz}$$

* Bin 1 represents: $62.5\,\text{Hz}$
* Bin 2 represents: $125\,\text{Hz}$
* ...
* Bin 31 represents: $1937.5\,\text{Hz}$

---

### 3. The Cooley-Tukey Radix-2 DIT FFT Algorithm
The Discrete Fourier Transform (DFT) has a calculation complexity of $O(N^2)$. The Cooley-Tukey FFT reduces this to $O(N \log N)$ by recursively splitting the $N$-point series into even and odd parts.

#### Step 1: Bit-Reversal Permutation
We reorder the input array indices based on their binary-reversed representation. For $N=8$, index $001$ ($1$) is swapped with index $100$ ($4$). This reorders the data arrays so that computed outputs merge correctly in the butterfly stages.

#### Step 2: Butterfly Computations
At each stage, pairs of data points are combined with a complex weighting factor called the **Twiddle Factor** ($W_N^k$):
$$W_N^k = e^{-j \frac{2\pi k}{N}} = \cos\left(\frac{2\pi k}{N}\right) - j \sin\left(\frac{2\pi k}{N}\right)$$

```
  x[u] ----------+----------> x[u] + Temp (Upper output)
                  \      /
                   \    /
                    \  /
                     \/
                     /\
                    /  \
                   /    \
                  /      \
  x[v] ---[ W_N^k ]-------+--> x[u] - Temp (Lower output)
```

#### Step 3: Magnitude Extraction
The output values are complex numbers ($A + jB$). To find the volume (amplitude) of a frequency bin, we calculate its vector magnitude:
$$\text{Magnitude} = \sqrt{\text{Real}^2 + \text{Imaginary}^2}$$

---

## 🔄 DSP Algorithms Comparison

| DSP Algorithm | Frequency Resolution | CPU Resource Cost | Output Information | Best Used For |
| :--- | :--- | :--- | :--- | :--- |
| **Radix-2 FFT (Our choice)** | **Full Spectrum (Equal bins)** | **Medium ($O(N \log N)$)** | **Whole spectrum amplitude map** | **Spectrum visualizers, acoustic analysis, structural monitoring** |
| **Goertzel Algorithm** | Single Target Frequency | Extremely Low ($O(N)$) | Real-time amplitude of one specific note | DTMF tone decoders, guitar tuners |
| **Hartley Transform (FHT)** | Full Spectrum | Low (Real numbers only) | Whole spectrum map | High-speed, real-time AVR sound processing |
| **Discrete Fourier (DFT)** | Full Spectrum | High ($O(N^2)$) | Whole spectrum map | Theoretical models, non-embedded math |

---

## 🛠️ Components Needed

* 1x Arduino Uno
* 1x Electret Microphone Module with Pre-amplifier (e.g., MAX4466 or KY-037/KY-038)
* 1x Breadboard & Jumper wires

---

## 🔌 Pin-to-Pin Wiring

> [!TIP]
> For the cleanest analog signal, power the microphone pre-amplifier from the Arduino **3.3V pin** instead of 5V. The 5V line contains high-frequency electrical noise from the USB interface and processor clock, which distorts the audio readings.

```
       +-----------------------+
       |   Microphone Board    |
       +-----------------------+
        VCC     GND     OUT
         |       |       |
 Arduino 3.3V   GND     A0
```

---

## 💻 How to Test & Validate

1. Wire the microphone module to **A0** as detailed.
2. Upload [Day_57_FFT_Spectrum.ino](file:///d:/Downloads/100%20days%20of%20Arduino/Day_57_FFT_Spectrum/Day_57_FFT_Spectrum.ino).
3. Open the **Serial Monitor** and set the baud rate to **115200** (critical for fast printing).
4. **Visual Testing**:
   * Generate a pure tone (e.g. whistle or play a $1000\,\text{Hz}$ tone on a phone sine-generator app).
   * You will see a single asterisk bar spike at the $1000\,\text{Hz}$ label:
     ```
     ================= FFT AUDIO SPECTRUM (0 - 2000 Hz) =================
     ...
     875 Hz   [*     ]
     938 Hz   [***   ]
     1000 Hz  [****************************************]
     1062 Hz  [****  ]
     1125 Hz  [*     ]
     ...
     ====================================================================
     ```

---

## 🛠️ Troubleshooting Guide

### Common Issues
* **The visualizer displays a constant high level across ALL frequency bins**:
  * The microphone output is floating or not connected. Verify the connections between the OUT pin on the mic and the A0 pin on the Arduino.
* **The visualizer displays a massive spike on the very first bin (0 Hz)**:
  * > [!NOTE]
    > **DC Offset**: Microphone modules output an audio signal centered around a bias voltage (typically half of VCC, e.g. $1.65\text{V}$ or $2.5\text{V}$). This offset appears in the Fourier analysis as a massive frequency amplitude at $0\,\text{Hz}$ (direct current). In our code, we subtract `512` from the raw analog readings to shift the wave to zero-center, and ignore Bin 0 during plotting to prevent this offset from skewing the graph.
* **The frequency spikes are wide and muddy instead of a single sharp line**:
  * This is a normal mathematical artifact known as **spectral leakage**. Because the sample block cut-off is sudden at $N=64$, frequencies that do not fall exactly on bin intervals leak into adjacent bins. To reduce this, you can apply a windowing function (such as a Hann or Hamming window) to the input array before executing the FFT.

## 🧠 Code Explanation

Let's break down how we built a Fast Fourier Transform (FFT) engine from scratch:

### 1. Deterministic Sampling at the Nyquist Rate
```cpp
while (micros() - tStart < SAMPLING_PERIOD_US) { }
```
- To analyze frequency, our samples must be perfectly spaced in time. We chose a 4000 Hz sample rate (one sample every 250us).
- `analogRead()` takes ~100us. We then trap the code in an empty `while` loop, monitoring `micros()`, and breaking out the *exact* microsecond the 250us timer hits. This guarantees perfect phase alignment!
- A 4000 Hz sample rate gives us a "Nyquist Frequency" of 2000 Hz. We can accurately detect any audio pitch up to 2000 Hz.

### 2. The Cooley-Tukey Butterfly Operation
```cpp
float tr = real[v] * wr - imag[v] * wi;
float ti = real[v] * wi + imag[v] * wr;
real[v] = real[u] - tr;
// ...
```
- The FFT algorithm breaks a massive 64-point calculation down into tiny pairs of 2, called "Butterflies".
- It multiplies the time-domain audio samples by complex "Twiddle Factors" (Sines and Cosines, represented by `wr` and `wi`).
- This recursively recombines the time-domain points until they mathematically morph into the frequency-domain magnitudes, showing us exactly how much bass, mid, and treble energy is in the audio signal!
