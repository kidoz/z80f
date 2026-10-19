# Changelog

## 1.0.1 — 2026-10-19

Patch release focused on timing fidelity and flag accuracy.

- Every T-state is now surfaced through `Bus::on_m_cycle`, including internal
  delay cycles, so hosts can layer per-T-state wait states and contention;
  cycle totals include the wait states returned by the bus.
- Interrupts taken while halted return to the instruction after `HALT`,
  including wrapping from `0xFFFF` to `0x0000`, and nested NMIs preserve
  `IFF2` for `RETN`.
- Block I/O instructions (`INI`/`IND`/`INIR`/`INDR` and the `OUT` family) set
  carry, half-carry, and parity correctly, and block instructions update
  `MEMPTR/WZ`.
- `SCF` and `CCF` derive undocumented flag bits 3/5 from the previous
  instruction's flag result through the internal Q latch.
- Memory `BIT` takes undocumented bits 3/5 from the high byte of the effective
  address, including address `0xFFFF`.
- Untaken conditional `CALL` instructions load `MEMPTR/WZ` from their target.
- `LD R,A` copies all eight bits of `A`.

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
