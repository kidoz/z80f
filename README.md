# Z80F

[![Language: C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/23)
[![Build: Meson](https://img.shields.io/badge/build-Meson-264478?logo=meson&logoColor=white)](https://mesonbuild.com/)
[![Tests: Catch2](https://img.shields.io/badge/tests-Catch2_v3-red)](https://github.com/catchorg/Catch2)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

A cycle-aware C++23 Zilog Z80 emulation core for ZX Spectrum, Sega Mega Drive / Genesis sound Z80, and other Z80-based systems.

**Z80F** stands for **Z80 Fidelity**: a small, reusable CPU core focused on correctness and clean emulator integration. It is a library, not a full emulator — hosts implement memory and I/O through the `z80f::Bus` interface.

## Project purpose

- Provide a single high-fidelity Z80 core that several emulators can embed.
- Stay machine-agnostic. Memory contention, peripherals, banking, and timing pacing live in the host.
- Expose a small, stable public API so hosts can pin to a version with confidence.
- Be testable on its own, without ROM files.

## Build

```bash
meson wrap install catch2
meson setup build
meson compile -C build
meson test -C build
```

Catch2 is fetched through Meson WrapDB; no system install is required.

Build options:

- `-Dtests=true|false` (default `true`)
- `-Dexamples=true|false` (default `true`)

## Basic API example

A complete, compilable host: a flat 64 KiB RAM, a 3-byte program that loads `A` and halts.

```cpp
#include <z80f/z80.hpp>

#include <array>
#include <cstdint>
#include <cstdio>

class FlatRamBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};

    std::uint8_t read_memory(std::uint16_t address) override {
        return memory[address];
    }
    void write_memory(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0xFF; }
    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}

    // Return extra wait T-states for this M-cycle. 0 means "no wait states".
    // ZX Spectrum / Mega Drive hosts would compute ULA contention or BUSREQ
    // delays here.
    int on_m_cycle(std::uint16_t /*address*/, int /*t_states*/) override { return 0; }
};

int main() {
    FlatRamBus bus;

    // Tiny program: LD A,0x42 ; HALT
    bus.memory[0x0000] = 0x3E;
    bus.memory[0x0001] = 0x42;
    bus.memory[0x0002] = 0x76;

    z80f::Z80 cpu(bus);
    cpu.reset();  // PC = 0, SP = 0xFFFF

    // Run until the CPU halts, capping at ~one ZX Spectrum frame (69888 T).
    int budget = 69888;
    while (!cpu.registers().halted && budget > 0) {
        budget -= cpu.step();
    }

    std::printf("A = 0x%02X, halted = %s, cycles = %llu\n",
                cpu.registers().a,
                cpu.registers().halted ? "yes" : "no",
                static_cast<unsigned long long>(cpu.cycle_counter()));
    return 0;
}
```

Expected output:

```text
A = 0x42, halted = yes, cycles = 11
```

For richer hosts (ROM/RAM split, contended memory, banked windows, peripherals), see
`examples/zx_spectrum_host.cpp` and `examples/megadrive_sound_z80_host.cpp`.

## ZX Spectrum integration

`examples/zx_spectrum_host.cpp` demonstrates wiring the core into a ZX Spectrum-style host:
64 KiB flat address space, ROM/RAM split, ULA/contention hook placeholder, port `0xFE` keyboard/border/speaker placeholder, and a 50 Hz frame loop driven by `cpu.run_for(T)`.

The example does not implement a full Spectrum — it shows where each host concern attaches to the core.

## Mega Drive sound Z80 integration

`examples/megadrive_sound_z80_host.cpp` demonstrates using the core as the Mega Drive sound CPU:
8 KiB Z80 RAM, a banked window into the 68000 address space, YM2612 and PSG I/O placeholders, and `BUSREQ`/`RESET` stubs.

The Z80 core never knows about YM2612, SN76489, the 68000, or the VDP. All of that lives in the host bus layer.

## Accuracy goals

- Full documented Z80 instruction set with CB / ED / DD / FD / DDCB / FDCB prefixes
- All standard registers, shadow registers, `IX`/`IY` (and their half-registers), `I`, `R`
- `IFF1`, `IFF2`, interrupt modes IM 0 / IM 1 / IM 2, NMI, `EI` delay, `HALT`
- Documented flag behavior including bits 3 and 5 of `F` where required
- Per-instruction T-state accounting, including taken vs not-taken branches and block-instruction repeat costs
- `MEMPTR/WZ` modeling where high-accuracy tests depend on it
- Snapshot save/load of CPU state

Memory contention is **not** baked into the core; hosts add wait states through `Bus::on_m_cycle`.

## Current limitations

- This early release covers the public API surface, register/flag model, and a working subset of the instruction set sufficient for the bundled tests. The remaining opcodes follow the same dispatch shape and are filled in incrementally.
- `MEMPTR/WZ` is tracked but not yet wired into every flag side-effect.
- ZEXDOC is integrated in the test suite; broader external suites such as ZEXALL
  and fusetest are not integrated yet.

## References

- Zilog Z80 CPU User Manual: <https://www.zilog.com/docs/z80/um0080.pdf>
- Meson built-in options: <https://mesonbuild.com/Builtin-options.html>
- Meson WrapDB packages: <https://mesonbuild.com/Wrapdb-projects.html>
- Catch2: <https://github.com/catchorg/Catch2>

## License

MIT. Copyright (c) 2026 Aleksandr Pavlov &lt;ckidoz@gmail.com&gt;. See `LICENSE` for the full text.
