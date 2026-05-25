// ZX Spectrum host example.
//
// Shows where each Spectrum-specific concern attaches to the z80f core. It is
// not a full Spectrum emulator: there is no ROM, no real ULA, no keyboard
// matrix, no border or speaker mixing. The intent is to demonstrate the host
// integration surface for a contended 48K Spectrum.

#include <z80f/snapshot.hpp>
#include <z80f/z80.hpp>

#include <array>
#include <cstdint>
#include <cstdio>

#include "z80f/bus.hpp"

namespace {

class SpectrumBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};
    std::uint8_t border = 0;
    std::uint8_t last_speaker = 0;

    std::uint8_t read_memory(std::uint16_t address) override { return memory[address]; }

    void write_memory(std::uint16_t address, std::uint8_t value) override {
        // ROM region (0x0000..0x3FFF) is read-only on a real Spectrum.
        if (address < 0x4000) {
            return;
        }
        memory[address] = value;
    }

    std::uint8_t read_io(std::uint16_t port) override {
        // ULA decodes by bit 0 = 0 (even ports). Keyboard rows are read from
        // the high byte. Returning 0xFF means "no keys pressed".
        if ((port & 1) == 0) {
            return 0xFF;
        }
        return 0xFF;  // open bus
    }

    void write_io(std::uint16_t port, std::uint8_t value) override {
        if ((port & 1) == 0) {
            // Bit 0..2: border colour. Bit 3: MIC out. Bit 4: speaker.
            border = static_cast<std::uint8_t>(value & 0x07);
            last_speaker = static_cast<std::uint8_t>((value >> 4) & 1);
        }
    }

    int on_m_cycle(std::uint16_t address, int /*t_states*/) override {
        return 0;
        // Contended memory: 0x4000..0x7FFF on the 48K machine. A real host
        // would apply a contention pattern derived from the ULA T-state
        // position within the frame. Here we simply count touches as a
        // placeholder for instrumentation.
        if (address >= 0x4000 && address <= 0x7FFF) {
            ++contended_accesses;
        }
    }

    std::uint64_t contended_accesses = 0;
};

// Spectrum runs the Z80 at ~3.5 MHz. A 50 Hz frame is 69888 T-states.
constexpr int k_spectrum_frame_t_states = 69888;

}  // namespace

int main() {
    SpectrumBus bus;
    z80f::Z80 cpu(bus);

    // A tiny synthetic "program" in RAM: write 0x07 (white border) and HALT.
    bus.memory[0x8000] = 0x3E;
    bus.memory[0x8001] = 0x07;  // LD A,7
    bus.memory[0x8002] = 0xD3;
    bus.memory[0x8003] = 0xFE;  // OUT (0xFE),A
    bus.memory[0x8004] = 0x76;  // HALT

    cpu.reset();
    z80f::Snapshot snapshot = cpu.save_snapshot();
    snapshot.registers.pc = 0x8000;
    snapshot.registers.iff1 = true;
    snapshot.registers.im = 1;
    cpu.load_snapshot(snapshot);

    // Run one frame, then raise the 50 Hz interrupt and release it.
    cpu.run_for(k_spectrum_frame_t_states);
    cpu.set_int_line(true);
    cpu.run_for(64);
    cpu.set_int_line(false);

    std::printf("border = %u\n", static_cast<unsigned>(bus.border));
    std::printf("speaker = %u\n", static_cast<unsigned>(bus.last_speaker));
    std::printf("contended accesses = %llu\n",
                static_cast<unsigned long long>(bus.contended_accesses));
    return 0;
}
