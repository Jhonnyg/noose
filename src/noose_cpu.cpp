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
static uint16_t address_temp;

static cpu::instruction_meta instruction_meta_table[] = {
    { "NOP",     cpu::FUNC_NOP },
    { "BIT",     cpu::FUNC_NOP },
    { "JMP",     cpu::FUNC_JMP },
    { "JMP_ABS", cpu::FUNC_NOP },
    { "STY",     cpu::FUNC_NOP },
    { "LDY",     cpu::FUNC_NOP },
    { "CPY",     cpu::FUNC_NOP },
    { "CPX",     cpu::FUNC_NOP },
    // 01
    { "ORA",     cpu::FUNC_NOP },
    { "AND",     cpu::FUNC_NOP },
    { "EOR",     cpu::FUNC_NOP },
    { "ADC",     cpu::FUNC_NOP },
    { "STA",     cpu::FUNC_NOP },
    { "LDA",     cpu::FUNC_NOP },
    { "CMP",     cpu::FUNC_NOP },
    { "SBC",     cpu::FUNC_NOP },
    // 10
    { "ASL",     cpu::FUNC_NOP },
    { "ROL",     cpu::FUNC_NOP },
    { "LSR",     cpu::FUNC_NOP },
    { "ROR",     cpu::FUNC_NOP },
    { "STX",     cpu::FUNC_STX },
    { "LDX",     cpu::FUNC_LDX },
    { "DEC",     cpu::FUNC_NOP },
    { "INC",     cpu::FUNC_NOP },
};

struct address_mode_lut_entry
{
    cpu::address_mode mode;
    const char*       mode_str;
};

static address_mode_lut_entry address_mode_lut[] =
{
    {cpu::MODE_IMMEDIATE,          "MODE_IMMEDIATE" },
    {cpu::MODE_ZEROPAGE,           "MODE_ZEROPAGE" },
    {cpu::MODE_UNUSED,             "MODE_UNUSED" },
    {cpu::MODE_ABSOLUTE,           "MODE_ABSOLUTE" },
    {cpu::MODE_UNUSED,             "MODE_UNUSED" },
    {cpu::MODE_ZEROPAGE_X_INDEXED, "MODE_ZEROPAGE_X_INDEXED" },
    {cpu::MODE_UNUSED,             "MODE_UNUSED" },
    {cpu::MODE_ABSOLUTE_X_INDEXED, "MODE_ABSOLUTE_X_INDEXED" },

    // CC 01
    {cpu::MODE_X_INDEXED_INDIRECT, "MODE_X_INDEXED_INDIRECT"},
    {cpu::MODE_ZEROPAGE,           "MODE_ZEROPAGE"},
    {cpu::MODE_IMMEDIATE,          "MODE_IMMEDIATE"},
    {cpu::MODE_ABSOLUTE,           "MODE_ABSOLUTE"},
    {cpu::MODE_INDIRECT_Y_INDEXED, "MODE_INDIRECT_Y_INDEXED"},
    {cpu::MODE_ZEROPAGE_X_INDEXED, "MODE_ZEROPAGE_X_INDEXED"},
    {cpu::MODE_ABSOLUTE_X_INDEXED, "MODE_ABSOLUTE_X_INDEXED"},
    {cpu::MODE_ABSOLUTE_X_INDEXED, "MODE_ABSOLUTE_X_INDEXED"},

    // CC 10
    {cpu::MODE_IMMEDIATE,          "MODE_IMMEDIATE"},
    {cpu::MODE_ZEROPAGE,           "MODE_ZEROPAGE"},
    {cpu::MODE_ACCUMULATOR,        "MODE_ACCUMULATOR"},
    {cpu::MODE_ABSOLUTE,           "MODE_ABSOLUTE"},
    {cpu::MODE_UNUSED,             "MODE_UNUSED"},
    {cpu::MODE_ZEROPAGE_X_INDEXED, "MODE_ZEROPAGE_X_INDEXED"},
    {cpu::MODE_UNUSED,             "MODE_UNUSED"},
    {cpu::MODE_ABSOLUTE_X_INDEXED, "MODE_ABSOLUTE_X_INDEXED"},
};

#define ADD_ACTION_COPY(type, f, t, b) \
    { \
        cpu::action::copy_##type ac = { .from = f, .to = t, .copy_behaviour = b }; \
        action_list_out[action_index++] = cpu::action(ac); \
    }

#define ADD_ACTION_READ(type, a, t, b) \
    { \
        cpu::action::read_##type ac = { .address = a, .to = t, .read_behaviour = b }; \
        action_list_out[action_index++] = cpu::action(ac); \
    }

#define ADD_ACTION_WRITE(type, f, a) \
    { \
        cpu::action::write_##type ac    = { .from = f, .address = a }; \
        action_list_out[action_index++] = cpu::action(ac); \
    }

#define ADD_ACTION_COPY_SHORT(f, t, b) (ADD_ACTION_COPY(short, f, t, b))
#define ADD_ACTION_COPY_BYTE(f, t, b)  (ADD_ACTION_COPY(byte, f, t, b))
#define ADD_ACTION_READ_BYTE(a, t, b)  (ADD_ACTION_READ(byte, a, t, b))
#define ADD_ACTION_WRITE_BYTE(f, a)    (ADD_ACTION_WRITE(byte, f, a))

static uint8_t fill_action_list_read(cpu::address_mode mode, cpu::action_address dst, cpu::action* action_list_out)
{
    uint8_t action_index = 0;
    switch(mode)
    {
        case cpu::MODE_ABSOLUTE:
        {
            /*
            ADD_ACTION_COPY_BYTE(cpu::ADDRESS_PC_PTR_ADVANCE, cpu::ADDRESS_TEMP_LO, cpu::COPY_NONE);
            ADD_ACTION_COPY_BYTE(cpu::ADDRESS_PC_PTR_ADVANCE, cpu::ADDRESS_TEMP_HI, cpu::COPY_NONE);
            ADD_ACTION_READ_BYTE(cpu::ADDRESS_TEMP, cpu::ADDRESS_X, cpu::READ_NONE);
            */
        } break;
        case cpu::MODE_IMMEDIATE:
        {
            ADD_ACTION_COPY_BYTE(cpu::ADDRESS_PC_PTR_ADVANCE, dst, cpu::COPY_NONE);
        } break;
        case cpu::MODE_ABSOLUTE_X_INDEXED: break;
        case cpu::MODE_ABSOLUTE_y_INDEXED: break;
        case cpu::MODE_IMPLIED: break;
        case cpu::MODE_INDIRECT: break;
        case cpu::MODE_X_INDEXED_INDIRECT: break;
        case cpu::MODE_INDIRECT_Y_INDEXED: break;
        case cpu::MODE_RELATIVE: break;
        case cpu::MODE_ZEROPAGE: break;
        case cpu::MODE_ZEROPAGE_X_INDEXED: break;
        case cpu::MODE_ZEROPAGE_Y_INDEXED: break;
        case cpu::MODE_UNUSED: break;
    }

    return action_index;
}

static uint8_t fill_action_list_write(cpu::address_mode mode, cpu::action_address from, cpu::action* action_list_out)
{
    uint8_t action_index = 0;
    switch(mode)
    {
        case cpu::MODE_ZEROPAGE:
        {
            ADD_ACTION_COPY_BYTE(cpu::ADDRESS_PC_PTR_ADVANCE, cpu::ADDRESS_TEMP_LO, cpu::COPY_NONE);
            ADD_ACTION_WRITE_BYTE(from, cpu::ADDRESS_TEMP_LO);
        } break;
    }
    return action_index;
}

static uint8_t fill_action_list(const cpu::instruction_meta meta, cpu::address_mode mode, cpu::action* action_list_out)
{
    uint8_t action_index = 0;

    switch(meta.type)
    {
        case cpu::FUNC_NOP:
            break;
        case cpu::FUNC_JMP:
        {
            // #  address R/W description
            //--- ------- --- -------------------------------------------------
            // 2    PC     R  fetch low address byte, increment PC
            // 3    PC     R  copy low address byte to PCL, fetch high address
            //                byte to PCH
            ADD_ACTION_COPY_SHORT(cpu::ADDRESS_PC_PTR_ADVANCE, cpu::ADDRESS_TEMP, cpu::COPY_NONE);
            ADD_ACTION_COPY_SHORT(cpu::ADDRESS_TEMP,           cpu::ADDRESS_PC,   cpu::COPY_NONE);
        } break;
        case cpu::FUNC_LDX:
        {
            // #  address R/W description
            //--- ------- --- ------------------------------------------
            // 2    PC     R  fetch low byte of address, increment PC
            // 3    PC     R  fetch high byte of address, increment PC
            // 4  address  R  read from effective address
            action_index += fill_action_list_read(mode, cpu::ADDRESS_X, action_list_out);
        } break;
        case cpu::FUNC_STX:
        {
            action_index += fill_action_list_write(mode, cpu::ADDRESS_X, action_list_out);
        } break;
    }

    return action_index;
}

#undef ADD_ACTION_COPY
#undef ADD_ACTION_COPY_SHORT
#undef ADD_ACTION_READ
#undef ADD_ACTION_READ_BYTE
#undef ADD_ACTION_WRITE_BYTE

static void do_action_copy_short(const cpu::action action)
{
    uint16_t data = 0x0;
    switch(action.copy_short_data.from)
    {
        case cpu::ADDRESS_PC_PTR_ADVANCE:
            data     = ((uint16_t) cpu::read_memory(cpu::pc + 1) << 8) | cpu::read_memory(cpu::pc);
            cpu::pc += 0x02;
            break;
        case cpu::ADDRESS_TEMP:
            data = address_temp;
            break;
    }

    switch(action.copy_short_data.to)
    {
        case cpu::ADDRESS_TEMP:
            address_temp = data;
            break;
        case cpu::ADDRESS_PC:
            cpu::pc = data;
            break;
    }
}

static void do_action(const cpu::action action)
{
    switch(action.id)
    {
        case cpu::ID_INCREMENT_PC:
            cpu::pc += 0x01;
            break;
        case cpu::ID_COPY_SHORT:
        {
            do_action_copy_short(action);
        } break;
        case cpu::ID_READ_BYTE:
        {
            uint8_t data = 0x0;
            switch(action.read_byte_data.address)
            {
                case cpu::ADDRESS_TEMP:
                    data = address_temp;
                    break;
            }

            switch(action.read_byte_data.to)
            {
                case cpu::ADDRESS_X:
                    cpu::x = data;
                    break;
            }

        } break;
        case cpu::ID_WRITE_BYTE:
        {
            uint8_t data = 0x0;
            switch(action.write_byte_data.from)
            {
                case cpu::ADDRESS_X:
                    data = cpu::x;
                    break;
            }

            switch(action.write_byte_data.address)
            {
                case cpu::ADDRESS_TEMP_LO:
                    cpu::write_memory((uint16_t) address_temp & 0xf, data);
                    break;
            }
        } break;
        case cpu::ID_COPY_BYTE:
        {
            uint8_t data = 0x0;
            switch(action.copy_byte_data.from)
            {
                case cpu::ADDRESS_PC_ADVANCE:
                    data     = cpu::pc;
                    cpu::pc += 0x01;
                    break;
                case cpu::ADDRESS_PC_PTR_ADVANCE:
                    data     = cpu::read_memory(cpu::pc);
                    cpu::pc += 0x01;
                    break;
            }

            switch(action.copy_byte_data.to)
            {
                case cpu::ADDRESS_X:
                    cpu::x = data;
                    break;
                case cpu::ADDRESS_TEMP_LO:
                    address_temp = (address_temp & 0xf0) + (uint16_t) data;
                    break;
            }

        } break;
        case cpu::ID_NOP:
            break;
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

uint8_t cpu::read_memory(uint16_t addr)
{
    if (addr >= 0x8000)
    {
        return cpu::prg_rom[(addr - 0x8000) % 0x4000];
    }
    else
    {
        assert(0 && "Not implemented!");
    }

    return 0;
}

void cpu::write_memory(uint16_t addr, uint8_t data)
{
    cpu::prg_rom[addr] = data;
}

cpu::address_mode cpu::get_address_mode(const cpu::instruction inst)
{
    uint8_t lut_index = inst.bits.bbb + inst.bits.cc * 8;
    return address_mode_lut[lut_index].mode;
}

cpu::instruction_meta cpu::get_instruction_meta(const cpu::instruction inst)
{
    return instruction_meta_table[inst.bits.aaa + inst.bits.cc * 8];
}

const char* cpu::get_address_mode_str(const cpu::instruction inst)
{
    uint8_t lut_index = inst.bits.bbb + inst.bits.cc * 8;
    return address_mode_lut[lut_index].mode_str;
}

cpu::instruction cpu::get_next_instruction()
{
    const uint8_t cc_bits  = 0x03;
    const uint8_t bbb_bits = 0x07;
    const uint8_t aaa_bits = 0x07;

    cpu::instruction inst = {};
    inst.code             = cpu::read_memory(pc);
    inst.bits.cc          = inst.code & cc_bits;
    inst.bits.bbb         = (inst.code >> 2) & bbb_bits;
    inst.bits.aaa         = (inst.code >> 5) & aaa_bits;
    inst.address_mode     = get_address_mode(inst);

    // fetching the instruction and increasing pc is always the first cycle
    cpu::instruction_meta meta  = get_instruction_meta(inst);
    inst.cycles[0]              = cpu::action(cpu::ID_INCREMENT_PC);
    inst.cycle_count            = 1 + fill_action_list(meta, get_address_mode(inst), &inst.cycles[1]);

    return inst;
}

void cpu::execute(const cpu::instruction inst)
{
    uint8_t cycle = 0;
    while(cycle < inst.cycle_count)
    {
        do_action(inst.cycles[cycle]);
        cycle++;
    }
}
