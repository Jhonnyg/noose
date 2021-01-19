#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "noose_internal.h"

using namespace noose;

uint8_t  cpu::prg_rom[32767];
uint8_t  cpu::ram[2048];
uint8_t  cpu::a;
uint8_t  cpu::x;
uint8_t  cpu::y;
uint8_t  cpu::p;
uint8_t  cpu::sp;
uint16_t cpu::pc;

// Extra address locations
static uint8_t address_temp;

static cpu::instruction_meta instruction_meta_table[] = {
    { "NOP",     cpu::FUNC_NOP, 0 },
    { "BIT",     cpu::FUNC_NOP, 0 },
    { "JMP",     cpu::FUNC_JMP, 3 },
    { "JMP_ABS", cpu::FUNC_NOP, 0 },
    { "STY",     cpu::FUNC_NOP, 0 },
    { "LDY",     cpu::FUNC_NOP, 0 },
    { "CPY",     cpu::FUNC_NOP, 0 },
    { "CPX",     cpu::FUNC_NOP, 0 },
    // 01
    { "ORA",     cpu::FUNC_NOP, 0 },
    { "AND",     cpu::FUNC_NOP, 0 },
    { "EOR",     cpu::FUNC_NOP, 0 },
    { "ADC",     cpu::FUNC_NOP, 0 },
    { "STA",     cpu::FUNC_NOP, 0 },
    { "LDA",     cpu::FUNC_NOP, 0 },
    { "CMP",     cpu::FUNC_NOP, 0 },
    { "SBC",     cpu::FUNC_NOP, 0 },
    // 10
    { "ASL",     cpu::FUNC_NOP, 0 },
    { "ROL",     cpu::FUNC_NOP, 0 },
    { "LSR",     cpu::FUNC_NOP, 0 },
    { "ROR",     cpu::FUNC_NOP, 0 },
    { "STX",     cpu::FUNC_NOP, 0 },
    { "LDX",     cpu::FUNC_LDX, 4 },
    { "DEC",     cpu::FUNC_NOP, 0 },
    { "INC",     cpu::FUNC_NOP, 0 },
};

static cpu::address_mode get_address_mode(const cpu::instruction inst)
{
    const cpu::address_mode addr_mode_lut[] = {
        // CC 00
        cpu::MODE_IMMEDIATE,
        cpu::MODE_ZEROPAGE,
        cpu::MODE_UNUSED,
        cpu::MODE_ABSOLUTE,
        cpu::MODE_UNUSED,
        cpu::MODE_ZEROPAGE_X_INDEXED,
        cpu::MODE_UNUSED,
        cpu::MODE_ABSOLUTE_X_INDEXED,

        // CC 01
        cpu::MODE_X_INDEXED_INDIRECT,
        cpu::MODE_ZEROPAGE,
        cpu::MODE_IMMEDIATE,
        cpu::MODE_ABSOLUTE,
        cpu::MODE_INDIRECT_Y_INDEXED,
        cpu::MODE_ZEROPAGE_X_INDEXED,
        cpu::MODE_ABSOLUTE_X_INDEXED,
        cpu::MODE_ABSOLUTE_X_INDEXED,

        // CC 10
        cpu::MODE_IMMEDIATE,
        cpu::MODE_ZEROPAGE,
        cpu::MODE_ACCUMULATOR,
        cpu::MODE_ABSOLUTE,
        cpu::MODE_UNUSED,
        cpu::MODE_ZEROPAGE_X_INDEXED,
        cpu::MODE_UNUSED,
        cpu::MODE_ABSOLUTE_X_INDEXED
    };

    switch(inst.bits.cc)
    {
        case 0: return addr_mode_lut[inst.bits.bbb];
        case 1: return addr_mode_lut[inst.bits.bbb + 8];
        case 2: return addr_mode_lut[inst.bits.bbb + 16];
    }

    return cpu::MODE_UNUSED;
}

static uint8_t read_memory(uint16_t addr)
{
    if (addr >= 0x8000)
    {
        return cpu::prg_rom[(addr - 0x8000) % 0x4000];
    }

    return 0;
}

static void fill_action_list(const cpu::instruction_meta meta, cpu::action* action_list_out)
{
    switch(meta.type)
    {
        case cpu::FUNC_JMP:
        {
            cpu::action::copy_byte ac0 = { .from = cpu::ADDRESS_PC,   .to = cpu::ADDRESS_TEMP,  .copy_behaviour = cpu::COPY_INCREMENT_PC };
            cpu::action::copy_byte ac1 = { .from = cpu::ADDRESS_TEMP, .to = cpu::ADDRESS_PC_LO, .copy_behaviour = cpu::COPY_SET_PC};
            action_list_out[0]         = cpu::action(ac0);
            action_list_out[1]         = cpu::action(ac1);
        } break;
    }
}

static void do_action(const cpu::action action, cpu::address_mode mode)
{
    switch(action.id)
    {
        case cpu::ID_INCREMENT_PC:
            cpu::pc += 0x01;
            break;
        case cpu::ID_COPY_BYTE:
        {
            uint8_t data = 0x0;
            switch(action.copy_byte_data.from)
            {
                case cpu::ADDRESS_PC:
                    data = read_address(mode, cpu::pc);
                    break;
                case cpu::ADDRESS_TEMP:
                    data = address_temp;
            }

            switch(action.copy_byte_data.to)
            {
                case cpu::ADDRESS_TEMP:
                    address_temp = data;
                    break;
                case cpu::ADDRESS_PC_LO:
                    cpu::pc = data;
                    break;
            }

            switch(action.copy_byte_data.copy_behaviour)
            {
                case cpu::COPY_INCREMENT_PC:
                    cpu::pc += 0x01;
                    break;
                case cpu::COPY_SET_PC:
                    cpu::pc = ((uint16_t) read_address(mode, cpu::pc) << 8) | data;
                    break;
            }

        } break;
    }
}

void cpu::initialize(const rom* rom)
{
    memset(ram, 0, sizeof(ram));
    memset(prg_rom, 0, sizeof(prg_rom));
    a  = 0;
    x  = 0;
    y  = 0;
    p  = 34;
    sp = 0xFD;
    pc = 0;

    memcpy(prg_rom, rom->data_prg, rom->header.page_count_prg * BLOCK_SIZE_PRG);
}

uint8_t cpu::read_address(cpu::address_mode mode, uint16_t op)
{
    switch(mode)
    {
        case cpu::MODE_ACCUMULATOR:        assert(0 && "read_instruction: accumulator not implemented"); return 0;
        case cpu::MODE_ABSOLUTE:           return read_memory(op);
        case cpu::MODE_ABSOLUTE_X_INDEXED: assert(0 && "read_instruction: absolute_x_indexed not implemented"); return 0;
        case cpu::MODE_ABSOLUTE_y_INDEXED: assert(0 && "read_instruction: absolute_y_indexed not implemented"); return 0;
        case cpu::MODE_IMMEDIATE:          return (uint8_t) op;
        case cpu::MODE_IMPLIED:            assert(0 && "read_instruction: implied not implemented"); return 0;
        case cpu::MODE_INDIRECT:           assert(0 && "read_instruction: indirect not implemented"); return 0;
        case cpu::MODE_X_INDEXED_INDIRECT: assert(0 && "read_instruction: x_indexed_indirect not implemented"); return 0;
        case cpu::MODE_INDIRECT_Y_INDEXED: assert(0 && "read_instruction: indirect_y_indexed not implemented"); return 0;
        case cpu::MODE_RELATIVE:           assert(0 && "read_instruction: relative not implemented"); return 0;
        case cpu::MODE_ZEROPAGE:           assert(0 && "read_instruction: zeropage not implemented"); return 0;
        case cpu::MODE_ZEROPAGE_X_INDEXED: assert(0 && "read_instruction: zeropage_x_indexed not implemented"); return 0;
        case cpu::MODE_ZEROPAGE_Y_INDEXED: assert(0 && "read_instruction: zeropage_y_indexed not implemented"); return 0;
        case cpu::MODE_UNUSED:             assert(0 && "read_instruction: unused not implemented"); return 0;
    }

    return 0;
}

cpu::instruction_meta cpu::get_instruction_meta(const cpu::instruction inst)
{
    switch(inst.bits.cc)
    {
        case 0: return instruction_meta_table[inst.bits.aaa];
        case 1: return instruction_meta_table[inst.bits.aaa + 8];
        case 2: return instruction_meta_table[inst.bits.aaa + 16];
    }

    return instruction_meta_table[0];
}

cpu::instruction cpu::get_next_instruction()
{
    const uint8_t cc_bits  = 0x03;
    const uint8_t bbb_bits = 0x07;
    const uint8_t aaa_bits = 0x07;

    cpu::instruction inst = {};
    inst.code             = read_memory(pc);
    inst.bits.cc          = inst.code & cc_bits;
    inst.bits.bbb         = (inst.code >> 2) & bbb_bits;
    inst.bits.aaa         = (inst.code >> 5) & aaa_bits;
    inst.address_mode     = get_address_mode(inst);

    return inst;
}

void cpu::execute(const cpu::instruction inst)
{
    uint8_t cycle = 0;

    // fetching the instruction and increasing pc is always the first cycle
    cpu::instruction_meta meta  = get_instruction_meta(inst);
    cpu::action action_list[16] = { cpu::action(cpu::ID_INCREMENT_PC) };
    fill_action_list(meta, &action_list[1]);

    while(cycle < meta.cycle_count)
    {
        do_action(action_list[cycle], inst.address_mode);
        cycle++;
    }
}



