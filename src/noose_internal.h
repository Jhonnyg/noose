#ifndef __NOOSE_INTERNAL_H__
#define __NOOSE_INTERNAL_H__

#include "noose.h"

namespace noose
{
    static const uint32_t BLOCK_SIZE_PRG = 16384;
    static const uint32_t BLOCK_SIZE_CHR = 8192;

    namespace cpu
    {
        enum address_mode
        {
            MODE_ACCUMULATOR        = 0,
            MODE_ABSOLUTE           = 1,
            MODE_ABSOLUTE_X_INDEXED = 2,
            MODE_ABSOLUTE_y_INDEXED = 3,
            MODE_IMMEDIATE          = 4,
            MODE_IMPLIED            = 5,
            MODE_INDIRECT           = 6,
            MODE_X_INDEXED_INDIRECT = 7,
            MODE_INDIRECT_Y_INDEXED = 8,
            MODE_RELATIVE           = 9,
            MODE_ZEROPAGE           = 10,
            MODE_ZEROPAGE_X_INDEXED = 11,
            MODE_ZEROPAGE_Y_INDEXED = 12,
            MODE_UNUSED             = 13,
        };

        enum func_type
        {
            FUNC_NOP = 0,
            FUNC_JMP = 1,
            FUNC_LDX = 2,
            FUNC_STX = 3,
            FUNC_BIT = 4,
            FUNC_JSR = 5,
        };

        enum cpu_flag
        {
            CPU_FLAG_CARRY       = 1,
            CPU_FLAG_ZERO        = 2,
            CPU_FLAG_IR_DISABLED = 4,
            CPU_FLAG_DECIMAL     = 8,
            // 2x NO EFFECT
            CPU_FLAG_OVERFLOW    = 64,
            CPU_FLAG_NEGATIVE    = 128,
        };

        enum action_address_bits
        {
            HI,
            LO,
        };

        enum action_address
        {
            ADDRESS_NONE,
            ADDRESS_A,
            ADDRESS_X,
            ADDRESS_Y,
            ADDRESS_PC,
            ADDRESS_PC_HI,
            ADDRESS_PC_LO,
            ADDRESS_PC_ADVANCE,
            ADDRESS_PC_PTR,
            ADDRESS_PC_PTR_ADVANCE,
            ADDRESS_TEMP,
            ADDRESS_TEMP_LO,
            ADDRESS_TEMP_HI,
        };

        /*
        enum action_address_short
        {
            ADDRESS_SHORT_NONE,
            ADDRESS_SHORT_PC,
            ADDRESS_SHORT_PC_PTR,
            ADDRESS_SHORT_PC_PTR_ADVANCE,
            ADDRESS_SHORT_TEMP,
        };
        */

        enum action_id
        {
            ID_NOP,
            ID_INCREMENT_PC,
            ID_COPY_BYTE,
            ID_COPY_SHORT,
            ID_READ_BYTE,
            ID_WRITE_BYTE,
        };

        enum action_behaviour_id
        {
            ID_NONE,
            ID_SET_FLAGS,
            ID_BIT_TEST,
        };

        struct action_behaviour
        {
            action_behaviour_id id;

            struct set_flags
            {
                uint8_t mask;
            };

            union
            {
                struct set_flags set_flags_data;
            };

            action_behaviour()                       : id(ID_NONE) {}
            action_behaviour(action_behaviour_id id) : id(id) {}
            action_behaviour(set_flags data)         : id(ID_SET_FLAGS), set_flags_data(data) {}

            static inline set_flags set_flags(uint8_t mask)
            {
                struct set_flags f = { .mask = mask };
                return f;
            }
        };

        struct action
        {
            action_id id;
            action_behaviour behaviour;

            struct copy_byte
            {
                action_address   from;
                action_address   to;
            };

            struct copy_short
            {
                action_address   from;
                action_address   to;
            };

            struct read_byte
            {
                action_address   address;
                action_address   to;
            };

            struct write_byte
            {
                action_address from;
                action_address address;
            };

            struct push_byte
            {
                action_address from;
            };

            union
            {
                struct copy_byte  copy_byte_data;
                struct copy_short copy_short_data;
                struct read_byte  read_byte_data;
                struct write_byte write_byte_data;
                struct push_byte  push_byte_data;
            };

            action()                                            : id(ID_NOP) {}
            action(action_id id)                                : id(id) {}
            action(copy_byte data, action_behaviour behaviour)  : id(ID_COPY_BYTE),  copy_byte_data(data), behaviour(behaviour)  {}
            action(copy_short data, action_behaviour behaviour) : id(ID_COPY_SHORT), copy_short_data(data), behaviour(behaviour) {}
            action(read_byte data, action_behaviour behaviour)  : id(ID_READ_BYTE),  read_byte_data(data), behaviour(behaviour)  {}
            action(write_byte data, action_behaviour behaviour) : id(ID_WRITE_BYTE), write_byte_data(data), behaviour(behaviour) {}
            action(push_byte data)                              : id(ID_WRITE_BYTE), push_byte_data(data) {}
        };

        struct s_instruction
        {
            address_mode address_mode;
            action       cycles[16];
            uint8_t      cycle_count;
            uint8_t      code;
            struct
            {
                uint8_t aaa : 3;
                uint8_t bbb : 3;
                uint8_t cc  : 2;
            } bits;
        };

        struct s_instruction_meta
        {
            const char* name;
            func_type   type;
        };

        typedef struct s_instruction_meta instruction_meta;
        typedef struct s_instruction      instruction;

        extern uint8_t  prg_rom[32767]; // $10000-$8000
        extern uint8_t  ram[2048];      // 2kb main RAM
        extern uint8_t  a;              // accumulator register
        extern uint8_t  x;              // index register x
        extern uint8_t  y;              // index register y
        extern uint8_t  p;              // cpu status flags
        extern uint8_t  sp;             // stack pointer
        extern uint16_t pc;             // program counter

        void             initialize(const noose::rom* rom);
        instruction      get_next_instruction();
        instruction_meta get_instruction_meta(const instruction inst);
        address_mode     get_address_mode(const cpu::instruction inst);
        const char*      get_address_mode_str(const cpu::instruction inst);
        uint8_t          read_memory(uint16_t addr);
        void             write_memory(uint16_t addr, uint8_t data);
        void             execute(const instruction inst);
    }
}

#endif
