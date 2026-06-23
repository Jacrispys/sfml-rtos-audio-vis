# SFML RTOS Audio Visualizer

A real-time audio spectrum visualizer written in modern C++17, built from scratch as a hands-on study of real-time systems concepts: lock-free concurrency, deterministic audio callbacks, and a hand-rolled FFT. All in service of making music look as good as it sounds.

<!-- TODO: add a screenshot or GIF of the visualizer running here -->
![Visualizer demo](docs/demo.gif)

## What this is

This project decodes and plays an audio file while simultaneously analyzing it in real time to drive a 6-band (configurable) frequency spectrum visualization. The interesting part isn't really the visuals, rather it's the architecture underneath them: three independent threads (the OS audio callback, an FFT analysis worker, and the render loop) cooperating without ever blocking each other, using lock-free data structures instead of mutexes.

It started as a learning project to get hands-on with real-time/embedded-adjacent concepts. Things like: deterministic timing, atomics, memory ordering, avoiding allocation in hot paths using a domain (audio) where the constraints are real and audible rather than theoretical.

## Architecture

```
[ Audio callback thread ]  --writes-->  [ Lock-free ring buffer ]  --reads-->  [ FFT worker thread ]
   (miniaudio, hard                     (atomic read/write indices,            (windowing, FFT,
    real-time deadline)                 single-producer/consumer)               magnitude calc)
                                                                                       |
                                                                                double-buffered
                                                                                atomic pointer swap
                                                                                       |
                                                                                       v
                                                                              [ Render thread (SFML) ]
                                                                               reads latest spectrum,
                                                                               draws bars at 60fps
```

Three threads, two lock-free hand-offs, zero mutexes:

- **Audio callback** — driven by the OS/sound device on a strict timing deadline. Decodes the file, mixes to mono, and writes samples into a ring buffer. Never blocks, never allocates.
- **`AudioRingBuffer`** — a single-producer/single-consumer lock-free circular buffer using `std::atomic` indices with carefully chosen memory orderings (`relaxed` for an index's own thread, `acquire`/`release` across threads). Full buffer conditions overwrite the oldest data rather than blocking the writer, since live visualization cares more about freshness than completeness.
- **`SpectrumAnalyzer`** — runs on its own thread, polling the ring buffer for new samples using overlapping FFT windows (a sliding window with a hop size smaller than the FFT length) so the spectrum updates more often than a fresh full window would otherwise allow, without sacrificing frequency resolution.
- **`FFT`** — a hand-implemented iterative Cooley-Tukey FFT (no external FFT library), including bit-reversal permutation and a Hamming window to reduce spectral leakage.
- **Spectrum publishing** — the analyzer publishes results to the render thread via double-buffering: two pre-allocated magnitude vectors that swap roles behind an atomic pointer, so the renderer always reads a complete, consistent snapshot without ever locking against the analyzer.
- **Render loop** — runs on the main thread via SFML, capped to a target framerate, reading whatever the latest published spectrum is every frame, and decoupled entirely from how often the analyzer actually produces a new one.

## Signal chain (raw FFT bins to what you see on screen)

The path from "FFT magnitude" to "bar height on screen" goes through several real audio-engineering steps, not just a direct mapping:

1. **Frequency banding** — FFT bins are grouped into a configurable number of bands, spaced evenly in octaves (log-frequency space) rather than linear Hz, matching how pitch is perceived.
2. **dB conversion** — raw linear magnitude is converted to decibels (`20 * log10(magnitude)`), since loudness perception is logarithmic.
3. **Frequency tilt compensation** — a per-band tilt is applied (relative to a reference frequency) to counteract the natural tendency of low frequencies to dominate a raw spectrum.
4. **Adaptive auto-gain** — a custom `AutoGain` stage tracks a moving "ceiling" per band (with independent attack/release rates, akin to a compressor) so the display adapts to the song's actual dynamic range instead of using fixed bounds. Per-band and shared (global) ceiling tracking can be blended continuously via a single parameter.
5. **Shaping curve** — a tunable exponent reshapes the normalized 0–1 range so the display sits lower at rest and requires genuine peaks to reach the top, rather than spending most of its time near the ceiling.
6. **Elastic smoothing** — an asymmetric exponential moving average (slow attack, fast release) eases the displayed bar toward its target each frame, giving the bars a springy, non-jittery feel without literal spring physics.

Every constant in this chain (band count, tilt amount, attack/release rates, shaping exponent) is exposed as a tunable parameter — there's no "correct" setting, just what looks best for a given genre/mix.

## Building

<!-- TODO: confirm/replace exact target name from CMakeLists.txt if different -->

### Prerequisites

- CMake 3.28+
- A C++17-compatible compiler (GCC, Clang, or MSVC)
- On Linux, SFML's system dependencies:

```bash
sudo apt update
sudo apt install \
    libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libfreetype-dev libflac-dev libvorbis-dev \
    libgl1-mesa-dev libegl1-mesa-dev libharfbuzz-dev \
    libmbedtls-dev libssh2-1-dev
```

Dependencies (SFML, miniaudio, dr_libs) are fetched automatically via CMake's `FetchContent` — no manual installation needed beyond the system libraries above.

### Build

```bash
cmake -B build
cmake --build build
```

The compiled binary will be in `build/bin`.

### Run

<!-- TODO: confirm exact invocation / how the audio file path is provided (hardcoded vs argv) -->
```bash
./build/bin/main
```

> Currently the audio file path is set in `main.cpp`. Replace it with the path to your own audio file (WAV/MP3/FLAC, anything `dr_libs`/miniaudio can decode) before building, or update the code to accept a command-line argument.

## Project structure

```
src/
├── main.cpp                  # ties everything together, owns the render loop
├── audio/
│   ├── AudioPlayer.h/.cpp    # decodes & plays audio via miniaudio; owns the real-time callback
│   ├── AudioRingBuffer.h     # lock-free single-producer/consumer ring buffer
│   └── FrequencyBand.h/.cpp  # band definitions, band-generation, bin averaging
└── analysis/
    ├── FFT.h/.cpp            # hand-rolled iterative Cooley-Tukey FFT
    ├── SpectrumAnalyzer.h    # worker thread: sliding-window FFT + result publishing
    └── AutoGain.h/.cpp       # adaptive per-band/shared dB normalization
```

## Configuration knobs

A few of the more interesting parameters to tune by ear, scattered through `main.cpp` and the relevant classes:

| Parameter | Where | Effect |
|---|---|---|
| `numBars` | `main.cpp` | Number of frequency bands/bars (bands are auto-generated, evenly spaced in octaves) |
| `fftLength` | `SpectrumAnalyzer` | FFT window size — trade-off between frequency resolution and update latency |
| `hopSize` | `SpectrumAnalyzer` | Overlap amount between consecutive FFT windows — should match or divide the audio device's callback buffer size |
| `attack` / `release` | `AutoGain` constructor | How quickly the adaptive ceiling rises to loud peaks vs. falls back down during quiet passages |
| `blend` | `AutoGain` constructor | 0.0 = fully per-band normalization, 1.0 = fully shared/global normalization |
| shaping exponent | `main.cpp` render loop | How aggressively mid-range values are pulled toward the floor before display |
| attack/release `rate` | `main.cpp` render loop | Visual easing speed of the displayed bar toward its target height |

### _Note: these configuration options will soon be moved into the main method to be set individually rather than having to search the code for the value to change._
## What this project deliberately avoids

In the spirit of the real-time-systems learning goal:

- **No mutexes anywhere in the audio→analysis→render path** — every hand-off is lock-free, using `std::atomic` with deliberately chosen memory orderings.
- **No allocation in the audio callback** — all buffers are pre-sized and reused.
- **No external FFT library** — the FFT is implemented from scratch (translated and adapted from an earlier Java implementation of the same algorithm).

## License

<!-- TODO: confirm — LICENSE.md exists in the repo; mirror its actual terms here -->
See [LICENSE.md](LICENSE.md).

## Acknowledgments

- [miniaudio](https://github.com/mackron/miniaudio) and [dr_libs](https://github.com/mackron/dr_libs) for audio decoding/playback
- [SFML](https://www.sfml-dev.org/) for windowing and rendering