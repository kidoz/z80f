#pragma once

#include <z80f/bus.hpp>
#include <z80f/flags.hpp>
#include <z80f/registers.hpp>
#include <z80f/snapshot.hpp>

#include <cstdint>

namespace z80f {

class Z80 {
public:
    explicit Z80(Bus& bus);

    void reset();

    // Execute one instruction. Returns the exact number of T-states consumed,
    // including any pending interrupt acknowledge that completed first.
    int step();

    // Execute instructions until at least `t_states` T-states have elapsed.
    // Returns the actual number of T-states consumed (>= `t_states`).
    int run_for(int t_states);

    void set_nmi_line(bool active);
    void set_int_line(bool active);

    [[nodiscard]] const Registers& registers() const noexcept { return regs_; }

    [[nodiscard]] Snapshot save_snapshot() const;
    void load_snapshot(const Snapshot& snapshot);

    [[nodiscard]] std::uint64_t cycle_counter() const noexcept { return cycles_; }

private:
    friend struct detail_z80_access;  // for test introspection

    Registers& unsafe_registers_mutable() noexcept { return regs_; }

    // Bus helpers (one T-state count surfaced through on_m_cycle per access).
    std::uint8_t read8(std::uint16_t addr);
    void write8(std::uint16_t addr, std::uint8_t value);
    std::uint8_t fetch_opcode();
    std::uint8_t fetch_immediate();
    std::uint16_t fetch_immediate16();
    std::uint8_t io_in(std::uint16_t port);
    void io_out(std::uint16_t port, std::uint8_t value);
    void push16(std::uint16_t value);
    std::uint16_t pop16();
    void inc_r();

    // Interrupts
    bool handle_nmi();
    bool handle_int();

    // Main dispatchers (defined in src/instructions.cpp / src/decode.cpp).
    int dispatch_unprefixed(std::uint8_t op);
    int dispatch_cb();
    int dispatch_ed();
    int dispatch_index(std::uint16_t& idx_ref, bool is_ix);
    int dispatch_index_cb(std::uint16_t& idx);

    // Flag helpers
    void alu_add(std::uint8_t value, bool with_carry);
    void alu_sub(std::uint8_t value, bool with_carry);
    void alu_and(std::uint8_t value);
    void alu_or(std::uint8_t value);
    void alu_xor(std::uint8_t value);
    void alu_cp(std::uint8_t value);
    std::uint8_t alu_inc(std::uint8_t value);
    std::uint8_t alu_dec(std::uint8_t value);
    void alu_add16(std::uint16_t& dest, std::uint16_t value);
    void alu_adc16(std::uint16_t value);
    void alu_sbc16(std::uint16_t value);
    void alu_daa();
    void alu_rlca();
    void alu_rla();
    void alu_rrca();
    void alu_rra();
    std::uint8_t cb_rlc(std::uint8_t v);
    std::uint8_t cb_rrc(std::uint8_t v);
    std::uint8_t cb_rl(std::uint8_t v);
    std::uint8_t cb_rr(std::uint8_t v);
    std::uint8_t cb_sla(std::uint8_t v);
    std::uint8_t cb_sra(std::uint8_t v);
    std::uint8_t cb_sll(std::uint8_t v);  // undocumented SLL/SL1
    std::uint8_t cb_srl(std::uint8_t v);
    void cb_bit(std::uint8_t bit, std::uint8_t v, std::uint16_t internal_addr);
    static std::uint8_t cb_set(std::uint8_t bit, std::uint8_t v);
    static std::uint8_t cb_res(std::uint8_t bit, std::uint8_t v);

    // Block ops
    void block_ld(int delta, bool repeat);
    void block_cp(int delta, bool repeat);
    void block_in(int delta, bool repeat);
    void block_out(int delta, bool repeat);

    // Jump / call helpers
    void jr_cond(bool cond);
    void jp_cond(bool cond);
    void call_cond(bool cond);
    void ret_cond(bool cond);
    void rst(std::uint16_t target);

    // State
    Bus* bus_;
    Registers regs_{};
    std::uint64_t cycles_ = 0;
    bool nmi_line_ = false;
    bool nmi_pending_ = false;
    bool int_line_ = false;
};

}  // namespace z80f
