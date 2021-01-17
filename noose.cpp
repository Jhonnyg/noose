#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noose.h"

namespace cpu
{
    enum func_type
    {
        NOP = 0x00,
        JMP = 0x02,
        LDX = 0x05,
    };

    enum address_mode
    {
        accumulator        = 0,
        absolute           = 1,
        absolute_x_indexed = 2,
        absolute_y_indexed = 3,
        immediate          = 4,
        implied            = 5,
        indirect           = 6,
        x_indexed_indirect = 7,
        indirect_y_indexed = 8,
        relative           = 9,
        zeropage           = 10,
        zeropage_x_indexed = 11,
        zeropage_y_indexed = 12,
        unused             = 13
    };

    typedef void (*func_ptr)(uint8_t);
    struct s_instruction_meta
    {
        const char* name;
        func_type   type;
        func_ptr    fn;
        uint8_t     operand_count;
        uint8_t     cycle_count;
    };

    struct s_instruction
    {
        s_instruction_meta meta;
        address_mode       address_mode;
        uint8_t            code;
        uint8_t            cc;
        uint8_t            bbb;
        uint8_t            aaa;
    };

    typedef struct s_instruction_meta instruction_meta;
    typedef struct s_instruction instruction;

    static uint8_t  prg_rom[32767]; // $10000-$8000
    static uint8_t  ram[2048];      // 2kb main RAM
    static uint8_t  a;              // accumulator register
    static uint8_t  x;              // index register x
    static uint8_t  y;              // index register y
    static uint8_t  p;              // cpu status flags
    static uint8_t  sp;             // stack pointer
    static uint16_t pc;             // program counter

    static instruction instruction_current;
    static uint8_t     instruction_data[16];

    static uint8_t read_memory(uint16_t addr)
    {
        if (addr >= 0x8000)
        {
            return prg_rom[(addr - 0x8000) % 0x4000];
        }

        return 0;
    }

    static uint8_t get_address_byte(address_mode mode, uint16_t addr)
    {
        switch(mode)
        {
            case accumulator: assert(0 && "get_address_byte: accumulator not implemented"); return 0;
            case absolute: return read_memory(addr);
            case absolute_x_indexed: assert(0 && "get_address_byte: absolute_x_indexed not implemented"); return 0;
            case absolute_y_indexed: assert(0 && "get_address_byte: absolute_y_indexed not implemented"); return 0;
            case immediate: assert(0 && "get_address_byte: immediate not implemented"); return 0;
            case implied: assert(0 && "get_address_byte: implied not implemented"); return 0;
            case indirect: assert(0 && "get_address_byte: indirect not implemented"); return 0;
            case x_indexed_indirect: assert(0 && "get_address_byte: x_indexed_indirect not implemented"); return 0;
            case indirect_y_indexed: assert(0 && "get_address_byte: indirect_y_indexed not implemented"); return 0;
            case relative: assert(0 && "get_address_byte: relative not implemented"); return 0;
            case zeropage: assert(0 && "get_address_byte: zeropage not implemented"); return 0;
            case zeropage_x_indexed: assert(0 && "get_address_byte: zeropage_x_indexed not implemented"); return 0;
            case zeropage_y_indexed: assert(0 && "get_address_byte: zeropage_y_indexed not implemented"); return 0;
            case unused: assert(0 && "get_address_byte: unused not implemented"); return 0;
        }

        return 0;
    }

    static uint16_t make_address(uint8_t hi, uint8_t lo)
    {
        return ((uint16_t) hi << 8) | lo;
    }

    static void func_fn_nop(uint8_t cycle)
    {
        // NOP
    }

    static void func_fn_jmp(uint8_t cycle)
    {
        assert(instruction_current.meta.type == JMP);
        switch(cycle)
        {
            case 0:
                pc += 0x01;
                break;
            case 1:
                instruction_data[0] = get_address_byte(instruction_current.address_mode, pc);
                pc += 0x01;
                break;
            case 2:
                instruction_data[1] = get_address_byte(instruction_current.address_mode, pc);
                pc                  = make_address(instruction_data[1], instruction_data[0]);
                break;
        }
    }

    static void func_fn_ldx(uint8_t cycle)
    {
        /*
        #  address R/W description
       --- ------- --- ------------------------------------------
        1    PC     R  fetch opcode, increment PC
        2    PC     R  fetch low byte of address, increment PC
        3    PC     R  fetch high byte of address, increment PC
        4  address  R  read from effective address
        */
        switch(cycle)
        {
            case 0:
                pc += 0x01;
                break;
            case 1:
                instruction_data[0] = get_address_byte(instruction_current.address_mode, pc);
                pc += 0x01;
                break;
            case 2:
                instruction_data[1] = get_address_byte(instruction_current.address_mode, pc);
                pc += 0x01;
                break;
            case 3:
                x = get_address_byte(instruction_current.address_mode, make_address(instruction_data[1], instruction_data[0]));
                break;
        }
    }

    static instruction_meta instruction_meta_table[] = {
        { "NOP", NOP, func_fn_nop, 0, 0 },
        { "BIT", NOP, func_fn_nop, 0, 0 },
        { "JMP", JMP, func_fn_jmp, 2, 3 },
        { "JMP_ABS", NOP, func_fn_nop, 2, 3 },
        { "STY", NOP, func_fn_nop, 2, 3 },
        { "LDY", NOP, func_fn_nop, 2, 3 },
        { "CPY", NOP, func_fn_nop, 2, 3 },
        { "CPX", NOP, func_fn_nop, 2, 3 },
        // 01
        { "ORA", NOP, func_fn_nop, 2, 3 },
        { "AND", NOP, func_fn_nop, 2, 3 },
        { "EOR", NOP, func_fn_nop, 2, 3 },
        { "ADC", NOP, func_fn_nop, 2, 3 },
        { "STA", NOP, func_fn_nop, 2, 3 },
        { "LDA", NOP, func_fn_nop, 2, 3 },
        { "CMP", NOP, func_fn_nop, 2, 3 },
        { "SBC", NOP, func_fn_nop, 2, 3 },
        // 10
        { "ASL", NOP, func_fn_nop, 2, 3 },
        { "ROL", NOP, func_fn_nop, 2, 3 },
        { "LSR", NOP, func_fn_nop, 2, 3 },
        { "ROR", NOP, func_fn_nop, 2, 3 },
        { "STX", NOP, func_fn_nop, 2, 3 },
        { "LDX", LDX, func_fn_ldx, 2, 3 },
        { "DEC", NOP, func_fn_nop, 2, 3 },
        { "INC", NOP, func_fn_nop, 2, 3 },
    };

    static void initialize()
    {
        memset(ram, 0, sizeof(ram));
        memset(prg_rom, 0, sizeof(prg_rom));
        a  = 0;
        x  = 0;
        y  = 0;
        p  = 34;
        sp = 0xFD;
        pc = 0;
    }

    static address_mode get_address_mode(const instruction inst)
    {
        const address_mode addr_mode_lut[] = {
            // CC 00
            immediate,
            zeropage,
            unused,
            absolute,
            unused,
            zeropage_x_indexed,
            unused,
            absolute_x_indexed,

            // CC 01
            x_indexed_indirect,
            zeropage,
            immediate,
            absolute,
            indirect_y_indexed,
            zeropage_x_indexed,
            absolute_y_indexed,
            absolute_x_indexed,

            // CC 10
            immediate,
            zeropage,
            accumulator,
            absolute,
            unused,
            zeropage_x_indexed,
            unused,
            absolute_x_indexed
        };

        switch(inst.cc)
        {
            case 0: return addr_mode_lut[inst.bbb];
            case 1: return addr_mode_lut[inst.bbb + 8];
            case 2: return addr_mode_lut[inst.bbb + 16];
        }

        return unused;
    }

    static instruction get_next_instruction()
    {
        const uint8_t cc_bits  = 0x03;
        const uint8_t bbb_bits = 0x07;
        const uint8_t aaa_bits = 0x07;
        uint8_t inst_hex = read_memory(pc);

        instruction inst;
        memset(&inst, 0, sizeof(inst));
        inst.code         = inst_hex;
        inst.cc           = inst_hex & cc_bits;
        inst.bbb          = (inst_hex >> 2) & bbb_bits;
        inst.aaa          = (inst_hex >> 5) & aaa_bits;
        inst.address_mode = get_address_mode(inst);

        switch(inst.cc)
        {
            case 0:
                inst.meta = instruction_meta_table[inst.aaa];
                break;
            case 1:
                inst.meta = instruction_meta_table[inst.aaa + 8];
                break;
            case 2:
                inst.meta = instruction_meta_table[inst.aaa + 16];
                break;
        }

        return inst;
    }

    static void execute(instruction inst)
    {
        instruction_current = inst;
        uint8_t cycle       = 0;
        while(cycle < instruction_current.meta.cycle_count)
        {
            instruction_current.meta.fn(cycle);
            // ppu::step(3);
            cycle++;
        }
    }
}

static const uint32_t prg_block_size = 16384;
static const uint32_t chr_block_size = 8192;
static char error_buffer[512] = {};

static bool set_last_error(const char* error_str)
{
    size_t error_str_size = strlen(error_str);
    assert(error_str_size < sizeof(error_buffer));
    memcpy(error_buffer, error_str, error_str_size);
    error_buffer[error_str_size] = '\0';
    return false;
}

static bool has_magic_number(noose::header header)
{
    const char magic_number[] = {'N','E', 'S', 0x1A};

    for (int i = 0; i < 4; ++i)
    {
        if (header.magic[i] != magic_number[i])
        {
            return false;
        }
    }

    return true;
}

static bool load_file(const char* path, uint8_t** buffer_out, uint32_t* buffer_size)
{
    FILE* f = fopen(path, "r");

    if (f == NULL)
    {
        return set_last_error("Unable to open file");
    }

    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    rewind(f);
    *buffer_out = (uint8_t*) malloc(f_size);
    *buffer_size = f_size;
    if (fread(*buffer_out, sizeof(uint8_t), f_size, f) != f_size)
    {
        return set_last_error("Couldn't read all bytes in ROM");
    }
    fclose(f);

    return true;
}

bool noose::load_rom(const char* path, noose::rom* output)
{
    uint8_t* buffer = 0;
    uint32_t buffer_size = 0;

    if (!load_file(path, &buffer, &buffer_size))
    {
        return false;
    }

    memcpy(&output->header, buffer, sizeof(noose::header));

    if (!has_magic_number(output->header))
    {
        return set_last_error("Invalid header, no magic number");
    }

    uint32_t cursor = sizeof(noose::header);

    if (noose::header::has_trainer_data(output->header))
    {
        uint32_t data_trainer_size = sizeof(output->data_trainer);
        memcpy(output->data_trainer, &buffer[cursor], data_trainer_size);
        cursor += data_trainer_size;
    }

    if (output->header.page_count_prg > 0)
    {
        uint32_t bytes_to_alloc = prg_block_size * output->header.page_count_prg;
        output->data_prg = (uint8_t*) malloc(bytes_to_alloc);
        memcpy(output->data_prg, &buffer[cursor], bytes_to_alloc);
        cursor += bytes_to_alloc;
    }

    if (output->header.page_count_chr > 0)
    {
        uint32_t bytes_to_alloc = output->header.page_count_chr * chr_block_size;
        output->data_chr = (uint8_t*) malloc(bytes_to_alloc);
        memcpy(output->data_chr, &buffer[cursor], bytes_to_alloc);
    }

    free(buffer);

    output->mapper_id = noose::header::mapper_number_lower(output->header) | (noose::header::mapper_number_higher(output->header) << 4);

    return true;
}

void noose::reset_rom(noose::rom* rom)
{
    if (rom->data_prg)
    {
        free(rom->data_prg);
    }

    if (rom->data_chr)
    {
        free(rom->data_chr);
    }

    memset(rom, 0, sizeof(*rom));
}

bool noose::verify_rom(const noose::rom* rom, const char* verify_log_path)
{
    cpu::initialize();

    // Start up state
    cpu::p  = 0x24; // Should be 34, but not for nestest apparently..
    cpu::pc = 0xC000;

    FILE* f = fopen(verify_log_path, "r");
    if (f == NULL)
    {
        return set_last_error("Unable to open verification log file");
    }

    memcpy(cpu::prg_rom, rom->data_prg, rom->header.page_count_prg * prg_block_size);

    uint32_t cycle_count = 7;

    printf("PC: %x\n", cpu::pc);

    char buffer_log[256];
    char buffer_noose[256];
    while(fgets(buffer_log, sizeof(buffer_log), f) != NULL)
    {
        cpu::instruction next = cpu::get_next_instruction();

        uint16_t pc   = cpu::pc;
        uint8_t reg_a = cpu::a;
        uint8_t reg_x = cpu::x;
        uint8_t reg_y = cpu::y;
        uint8_t p     = cpu::p;
        uint8_t sp    = cpu::sp;
        uint8_t ppu_x = 0;
        uint8_t ppu_y = 0;

        cpu::execute(next);

        char op0[3] = "  ";
        char op1[3] = "  ";
        char instruction_str[32];

        sprintf(instruction_str, "%s", next.meta.name);

        if (next.meta.operand_count > 0)
        {
            sprintf(op0, "%X", cpu::read_memory(pc + 1));
        }

        if (next.meta.operand_count > 1)
        {
            sprintf(op1, "%X", cpu::read_memory(pc + 2));
        }

        sprintf(buffer_noose, "%X  %X %s %s  %-32sA:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%d\n",
            pc, next.code, op0, op1, instruction_str, reg_a, reg_x, reg_y, p, sp, ppu_x, ppu_y, cycle_count);

        printf("%s", buffer_noose);
        printf("%s", buffer_log);

        cycle_count += next.meta.cycle_count;
    }

    fclose(f);
    return true;
}

void noose::debug(const char* debug_str)
{
    printf("[DEBUG] %s\n", debug_str);
}

void noose::error(const char* error_str)
{
    printf("[ERROR] %s\n", error_str);
}

void noose::print_help()
{
    printf("\n");
    printf("To use, call noose like this:\n");
    printf("noose <path-to-nes-file>\n");
}

void noose::print_header(const noose::header header)
{
    const char* tv_system_lut[] = {"NTCS", "PAL", "Dual Compatible"};

    printf("Magic                          : %s\n", header.magic);
    printf("Page count (PRG)               : %d\n", header.page_count_prg);
    printf("Page count (CHR)               : %d\n", header.page_count_chr);

    // Flags 6
    printf("Flag 6 (Nametable Mirror Mode) : %s\n", noose::header::nametable_mirroring_mode(header) ? "Vertical" : "Horizontal");
    printf("Flag 6 (Battery Backed PRG)    : %s\n", noose::header::battery_backed_prg(header) ? "True" : "False");
    printf("Flag 6 (Has Trainer Data)      : %s\n", noose::header::has_trainer_data(header) ? "True" : "False");
    printf("Flag 6 (Ignore Mirror Control) : %s\n", noose::header::ignore_mirror_control(header) ? "Ignore" : "Four-screen VRAM");
    printf("Flag 6 (Mapper Number Lower)   : %d\n", noose::header::mapper_number_lower(header));
    // Flags 7
    printf("Flag 7 (VS Unisystem)          : %s\n", noose::header::vs_unisystem(header) ? "True" : "False");
    printf("Flag 7 (Playchoice 10)         : %s\n", noose::header::playchoice_10(header) ? "True" : "False");
    printf("Flag 7 (NES 2.0)               : %s\n", noose::header::nes_2_0_bits(header) ? "True" : "False");
    printf("Flag 7 (Mapper Number Higher)  : %d\n", noose::header::mapper_number_higher(header));
    // Flags 8
    printf("Flag 8 (PRG RAM Size)          : %d\n", noose::header::prg_ram_size(header));
    // Flags 9
    printf("Flag 9 (TV System)             : %s\n", noose::header::tv_system_1(header) ? "NTCS" : "PAL");
    // Flags 10
    printf("Flag 10 (TV System)            : %s\n", tv_system_lut[noose::header::tv_system_2(header)]);
    printf("Flag 10 (PRG RAM)              : %s\n", noose::header::prg_ram(header) ? "True" : "False");
    printf("Flag 10 (Bus Conflict)         : %s\n", noose::header::bus_conflict(header) ? "Has Conflict" : "No Conflict");
}

const char* noose::last_error()
{
    return error_buffer;
}
