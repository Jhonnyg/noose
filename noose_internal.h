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
        };

        enum action_address
        {
            ADDRESS_A,
            ADDRESS_X,
            ADDRESS_Y,
            ADDRESS_PC_HI,
            ADDRESS_PC_LO,
            ADDRESS_PC,
            ADDRESS_TEMP
        };

        enum action_id
        {
            ID_NOP,
            ID_INCREMENT_PC,
            ID_COPY_BYTE,
        };

        enum action_copy_behaviour
        {
            COPY_NONE,
            COPY_INCREMENT_PC,
            COPY_SET_PC,
        };

        struct action
        {
            action_id id;

            struct copy_byte
            {
                action_address        from;
                action_address        to;
                action_copy_behaviour copy_behaviour;
            };

            union
            {
                struct copy_byte copy_byte_data;
            };

            action()               : id(ID_NOP) {}
            action(action_id id)   : id(id) {}
            action(copy_byte data) : id(ID_COPY_BYTE), copy_byte_data(data) {}
        };

        struct s_instruction
        {
            address_mode address_mode;
            uint8_t code;
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
            uint8_t     cycle_count;
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
        uint8_t          read_address(cpu::address_mode mode, uint16_t op);
        void             execute(const instruction inst);
    }
}

#endif
