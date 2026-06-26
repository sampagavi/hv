# llm-tracer

Local LLM Instrumentation, Tracing, and Replay Platform — a non-invasive telemetry TUI for transformer models.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Model / Simulator thread                                       │
│  ┌──────────────┐   LayerPacket   ┌─────────────────────────┐  │
│  │ MockModel /  │ ──────────────► │  RingBuffer<LayerPacket>│  │
│  │ Replayer     │                 │  (capacity 512)         │  │
│  └──────────────┘                 └───────────┬─────────────┘  │
│        │                                      │ peek()         │
│        │ on_packet                            ▼                │
│        ▼                          ┌─────────────────────────┐  │
│  ┌──────────────┐                 │  TUI render thread      │  │
│  │ Anomaly      │ ──────────────► │  ftxui ScreenInteractive│  │
│  │ Detector     │  AnomalyEvent   │  ~30fps via PostEvent   │  │
│  └──────────────┘                 └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘

Hook / intercept pattern (non-invasive):
  ScopeHook guard = LayerInterceptor.enter(...)
  // ← real or mock forward pass runs here
  guard.finish(stats)   // fires callback with timing + stats
```

## Panels

```
┌────────────────────┐ ┌────────────────────────────────────────┐
│ Panel 1: Topology  │ │ Panel 2: Live Packet Stream            │
│ (scrollable tree)  │ │ (auto-scroll, freeze on nav)           │
├────────────────────┴─┴────────────────────────────────────────┤
│ Panel 3: Attention Matrix Visualizer (full width)             │
├───────────────────────────────┬───────────────────────────────┤
│ Panel 4: Runtime Metrics      │ Panel 5: Anomaly Ledger       │
└───────────────────────────────┴───────────────────────────────┘
```

## Build

**Requirements:** CMake ≥ 3.20, C++17 compiler, internet access (ftxui fetched automatically)

```bash
# Using Make wrapper
make build        # configure + compile
make run          # build and launch live mode
make replay FILE=capture.llmt   # playback a saved capture
make clean        # remove build directory

# Or directly with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/llm-tracer
./build/llm-tracer --replay capture.llmt
```

## Keybindings

| Key         | Action                                          |
|-------------|-------------------------------------------------|
| `Tab`       | Cycle focus between panels                      |
| `j` / `k`   | Navigate topology up/down                       |
| `Enter`     | Expand/collapse topology node                   |
| `h/j/k/l`   | Pan attention matrix viewport                   |
| Arrow keys  | Pan attention matrix / scroll stream            |
| `+` / `-`   | Adjust attention contrast threshold             |
| `f`         | Toggle fullscreen on attention panel            |
| `r`         | Clear ring buffer and reset display             |
| `Ctrl+S`    | Save current capture to `capture.llmt`          |
| `Space`     | Pause/resume replay (replay mode only)          |
| `←` / `→`   | Step frame-by-frame (replay mode only)          |
| `?`         | Toggle help overlay                             |
| `q` / `Q`   | Quit                                            |

## Live Mode vs Replay Mode

**Live mode** (default): starts the mock LLM simulator which generates packets simulating a llama-3-8b topology (embed → 32× attention+MLP+LayerNorm) at ~100ms/layer intervals with randomized latency, sparsity, and occasional anomaly injections.

**Replay mode**: load a previously saved `.llmt` binary capture and play it back at original timing. Supports pause, resume, and frame stepping.

## Hook / Intercept Mechanism

The `LayerInterceptor` class demonstrates the non-invasive proxy pattern:

1. **`ScopeHook` RAII guard** wraps the boundary of any layer's forward pass — `enter()` records the high-resolution start timestamp; `finish()` computes elapsed time and fires the packet callback.
2. **In production**, the shared library (`libllm_hook.so`) would be injected via `LD_PRELOAD` (Linux) or `DYLD_INSERT_LIBRARIES` (macOS). It exports symbol stubs that shadow PyTorch/libtorch operator names, insert the RAII guard, then delegate to the real symbol via `dlsym(RTLD_NEXT, ...)`.
3. **No model source modification required** — attachment is purely at the dynamic linker level.

## Ring Buffer

`RING_BUFFER_CAPACITY` (default 512) is a compile-time constant in `src/buffer/ring_buffer.hpp`. Change it there to adjust memory footprint. The anomaly log uses a separate buffer of capacity 128 (`ANOMALY_BUFFER_CAPACITY` in `src/anomaly/detector.hpp`).

Thread safety: a single `std::mutex` protects all read/write operations. For higher throughput, replace with a lock-free SPSC queue (the interface is identical).

## Binary Capture Format (`.llmt`)

See `src/replay/serializer.hpp` for the full format spec. Header: 4-byte magic `LLMT`, 4-byte version, 8-byte packet count. Each packet is a fixed-layout binary record followed by length-prefixed strings and float arrays. Little-endian throughout.
