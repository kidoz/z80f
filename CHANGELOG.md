# Changelog

## 1.0.0 — 2026-08-19

First stable release.

- Full documented Z80 instruction set with CB / ED / DD / FD / DDCB / FDCB
  prefixes, verified by the ZEXDOC instruction exerciser.
- Undocumented flag bits 3/5 and undocumented instructions (`SLL`, IX/IY half
  registers), verified by the ZEXALL instruction exerciser.
- NMI, IM 0 / IM 1 / IM 2, `EI` delay, `HALT`, and INT line/pulse semantics
  with exact T-state accounting, including interrupt acknowledge costs.
- `MEMPTR/WZ` modeling, snapshot save/load including interrupt line state,
  cycle counters with since-reset deltas.
- Host integration via the `z80f::Bus` interface with per-M-cycle wait-state
  hook; ZX Spectrum and Mega Drive sound-CPU example hosts.
- Meson install target with pkg-config file and versioned shared library.
- CI matrix: Linux GCC, Linux Clang, macOS Clang, Windows MSVC.
