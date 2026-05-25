// Mega Drive / Genesis sound Z80 host example.
//
// The Mega Drive uses a Z80 as the sound CPU. It has 8 KiB of dedicated RAM
// at 0x0000..0x1FFF, memory-mapped access to the YM2612 FM chip at
// 0x4000..0x4003, a PSG (SN76489) latch at 0x7F11, and a banked 32 KiB window
// at 0x8000..0xFFFF into the 68000 address space. This file demonstrates only
// where each peripheral attaches to the z80f core. No real audio is produced.

#include <z80f/z80.hpp>

#include <array>
#include <cstdint>
#include <cstdio>

#include "z80f/bus.hpp"

namespace {

class MegadriveSoundBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x2000> z80_ram{};
    std::uint16_t bank_register = 0;  // selects which 32K window into 68K space
    bool bus_request = false;         // 68000 has taken the Z80 bus
    bool reset_line = false;          // 68000 holding Z80 in reset
    std::uint8_t last_ym_address = 0;
    std::uint8_t last_ym_data = 0;
    std::uint8_t last_psg = 0;

    std::uint8_t read_memory(std::uint16_t address) override {
        if (address < 0x2000) {
            return z80_ram[address];
        }
        if (address < 0x4000) {
            return z80_ram[address - 0x2000];  // mirror
        }
        if (address >= 0x4000 && address <= 0x4003) {
            // YM2612 status read (busy/timer flags). Stub returns 0.
            return 0x00;
        }
        if (address == 0x7F11) {
            return last_psg;
        }
        if (address >= 0x8000) {
            // Banked window into 68000 memory. A real implementation would
            // resolve through the bank register into ROM / 68K RAM / VDP.
            (void)bank_register;
            return 0xFF;
        }
        return 0xFF;
    }

    void write_memory(std::uint16_t address, std::uint8_t value) override {
        if (address < 0x2000) {
            z80_ram[address] = value;
            return;
        }
        if (address < 0x4000) {
            z80_ram[address - 0x2000] = value;
            return;
        }
        if (address == 0x4000 || address == 0x4002) {
            last_ym_address = value;  // address-latch port
            return;
        }
        if (address == 0x4001 || address == 0x4003) {
            last_ym_data = value;  // data port
            return;
        }
        if (address == 0x7F11) {
            last_psg = value;
            return;
        }
        if (address == 0x6000) {
            // Bank register: each write shifts in one bit of the 9-bit window
            // selector. Here we just OR the value as a placeholder.
            bank_register = static_cast<std::uint16_t>(
                (bank_register >> 1U) | (static_cast<std::uint16_t>(value & 1U) << 8U));
            return;
        }
    }

    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0xFF; }
    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}

    int on_m_cycle(std::uint16_t /*address*/, int /*t_states*/) override {
        return 0;
        // Mega Drive sound Z80 has no contention pattern. A host might use
        // this hook to schedule YM2612 sample generation or PSG state updates.
    }
};

}  // namespace

int main() {
    MegadriveSoundBus bus;
    z80f::Z80 cpu(bus);

    // Tiny test program: write a YM2612 register, write to PSG, then HALT.
    bus.z80_ram[0x0000] = 0x3E;
    bus.z80_ram[0x0001] = 0x22;  // LD A,0x22
    bus.z80_ram[0x0002] = 0x32;
    bus.z80_ram[0x0003] = 0x00;
    bus.z80_ram[0x0004] = 0x40;  // LD (0x4000),A
    bus.z80_ram[0x0005] = 0x3E;
    bus.z80_ram[0x0006] = 0xAA;  // LD A,0xAA
    bus.z80_ram[0x0007] = 0x32;
    bus.z80_ram[0x0008] = 0x11;
    bus.z80_ram[0x0009] = 0x7F;  // LD (0x7F11),A
    bus.z80_ram[0x000A] = 0x76;  // HALT

    cpu.reset();

    // On a real Mega Drive the host would only step the Z80 while BUSREQ
    // and RESET are released. We model that gate explicitly.
    while (!bus.bus_request && !bus.reset_line && !cpu.registers().halted) {
        cpu.step();
    }

    std::printf("YM2612 reg %02X = %02X\n", static_cast<unsigned>(bus.last_ym_address),
                static_cast<unsigned>(bus.last_ym_data));
    std::printf("PSG latch = %02X\n", static_cast<unsigned>(bus.last_psg));
    return 0;
}
