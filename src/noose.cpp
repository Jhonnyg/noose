#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noose.h"

#include "noose_internal.h"

static struct s_pgm_error
{
    char msg[512];
    s_pgm_error* next;
} error_head = {};

static void add_error(const char* error_str)
{
    s_pgm_error* error = (s_pgm_error*) malloc(sizeof(s_pgm_error));

    s_pgm_error* parent = &error_head;

    while(parent->next)
    {
        parent = parent->next;
    }

    parent->next = error;
    error->next = 0;

    size_t error_str_size = strlen(error_str);
    assert(error_str_size < sizeof(error->msg));
    memcpy(error->msg, error_str, error_str_size);
    error->msg[error_str_size] = '\0';
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
        add_error("Unable to open file");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    rewind(f);
    *buffer_out = (uint8_t*) malloc(f_size);
    *buffer_size = f_size;
    if (fread(*buffer_out, sizeof(uint8_t), f_size, f) != f_size)
    {
        add_error("Couldn't read all bytes in ROM");
        return false;
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
        add_error("Invalid header, no magic number");
        return false;
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
        uint32_t bytes_to_alloc = output->header.page_count_prg * BLOCK_SIZE_PRG;
        output->data_prg = (uint8_t*) malloc(bytes_to_alloc);
        memcpy(output->data_prg, &buffer[cursor], bytes_to_alloc);
        cursor += bytes_to_alloc;
    }

    if (output->header.page_count_chr > 0)
    {
        uint32_t bytes_to_alloc = output->header.page_count_chr * BLOCK_SIZE_CHR;
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

static inline uint8_t dbg_write_instruction_to_buffer_one_op(const noose::cpu::instruction_meta meta, char* buffer)
{
    int op0 = noose::cpu::read_memory(noose::cpu::pc + 1);
    return sprintf(buffer, "%02X     %s #$%02X", op0, meta.name, op0);
}

static inline uint8_t dbg_write_instruction_to_buffer_two_op(const noose::cpu::instruction_meta meta, char* buffer)
{
    int op0 = noose::cpu::read_memory(noose::cpu::pc + 1);
    int op1 = noose::cpu::read_memory(noose::cpu::pc + 2);
    return sprintf(buffer, "%02X %02X  %s $%02X%02X", op0, op1, meta.name, op1, op0);
}

static uint8_t dbg_write_instruction_to_buffer(const noose::cpu::instruction i, char* buffer)
{
    noose::cpu::instruction_meta meta = noose::cpu::get_instruction_meta(i);

    switch (meta.type)
    {
        case noose::cpu::FUNC_JMP:
        {
            return dbg_write_instruction_to_buffer_two_op(meta, buffer);
        }
        case noose::cpu::FUNC_LDX:
        {
            int op0 = noose::cpu::read_memory(noose::cpu::pc + 1);
            if (noose::cpu::get_address_mode(i) == noose::cpu::MODE_IMMEDIATE)
            {
                return sprintf(buffer, "%02X     %s #$%02X", op0, meta.name, op0);
            }
            else
            {
                int op1 = noose::cpu::read_memory(noose::cpu::pc + 2);
                return sprintf(buffer, "%02X %02X  %s $%02X%02X", op0, op1, meta.name, op1, op0);
            }
        }
        case noose::cpu::FUNC_STX:
        {
            int op0 = noose::cpu::read_memory(noose::cpu::pc + 1);
            if (noose::cpu::get_address_mode(i) == noose::cpu::MODE_ZEROPAGE)
            {
                return sprintf(buffer, "%02X     %s $%02X = %02X", op0, meta.name, op0, noose::cpu::x);
            }
        }
        case noose::cpu::FUNC_JSR:
        {
            return dbg_write_instruction_to_buffer_two_op(meta, buffer);
        }
        case noose::cpu::FUNC_NOP:
        {
            return dbg_write_instruction_to_buffer_one_op(meta, buffer);
        }
    }

    return 0;
}

static void print_debug_instruction(const noose::cpu::instruction inst)
{
    noose::cpu::instruction_meta meta = noose::cpu::get_instruction_meta(inst);
    printf("Instruction, Address Mode, Cycle count: %s, %s, %d\n", meta.name, get_address_mode_str(inst), inst.cycle_count);
}

bool noose::verify_rom(const noose::rom* rom, const char* verify_log_path)
{
    noose::cpu::initialize(rom);

    // Start up state
    noose::cpu::p  = 0x24; // Should be 34, but not for nestest apparently..
    noose::cpu::pc = 0xC000;

    FILE* f = fopen(verify_log_path, "r");
    if (f == NULL)
    {
        add_error("Unable to open verification log file");
        return false;
    }

    uint32_t cycle_count = 7;
    bool abort = false;
    char buffer_log[256];
    char buffer_noose[256];
    while(fgets(buffer_log, sizeof(buffer_log), f) != NULL && !abort)
    {
        noose::cpu::instruction next = noose::cpu::get_next_instruction();

        uint16_t pc   = cpu::pc;
        uint8_t reg_a = cpu::a;
        uint8_t reg_x = cpu::x;
        uint8_t reg_y = cpu::y;
        uint8_t p     = cpu::p;
        uint8_t sp    = cpu::sp;
        uint8_t ppu_x = 0;
        uint8_t ppu_y = 0;

        char instruction_str[40] = {};
        dbg_write_instruction_to_buffer(next, instruction_str);

        noose::cpu::execute(next);

        print_debug_instruction(next);

        #define COLOR_NRM  "\x1B[0m"
        #define COLOR_RED  "\x1B[31m"
        #define COLOR_GRN  "\x1B[32m"
        #define COLOR_YEL  "\x1B[33m"

        uint16_t cursor = 0;
        uint64_t last_color = (uint64_t) &COLOR_NRM;

        sprintf(buffer_noose, "%04X  %02X %-39sA:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%d",
            pc, next.code, instruction_str, reg_a, reg_x, reg_y, p, sp, ppu_x, ppu_y, cycle_count);

        while(buffer_log[cursor] && cursor < strlen(buffer_noose))
        {
            if (cursor >= 74 && cursor < 86)
            {
                if (last_color != (uint64_t) &COLOR_YEL)
                {
                    printf("%s", COLOR_YEL);
                    last_color = (uint64_t) &COLOR_YEL;
                }
                printf("%c", buffer_log[cursor]);
            }
            else if (buffer_noose[cursor] == buffer_log[cursor])
            {
                if (last_color != (uint64_t) &COLOR_GRN)
                {
                    printf("%s", COLOR_GRN);
                    last_color = (uint64_t) &COLOR_GRN;
                }

                printf("%c", buffer_log[cursor]);
            }
            else
            {
                if (last_color != (uint64_t) &COLOR_RED)
                {
                    printf("%s", COLOR_RED);
                    last_color = (uint64_t) &COLOR_RED;
                }

                printf("%c", buffer_log[cursor]);
                abort = 1;
            }

            cursor++;
        }

        printf("%s\n", COLOR_NRM);

        if (abort)
        {
            add_error("Input string mismatch:");
            add_error(buffer_noose);
        }

        #undef COLOR_NRM
        #undef COLOR_RED
        #undef COLOR_GRN
        #undef COLOR_YEL

        cycle_count += next.cycle_count;
    }

    fclose(f);
    return !abort;
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

static char error_buffer[512];

bool noose::has_errors()
{
    return error_head.next != 0;
}

const char* noose::last_error()
{
    s_pgm_error* e = error_head.next;
    if (e)
    {
        size_t len = strlen(e->msg);
        assert(len < sizeof(error_buffer));
        memcpy(error_buffer, e->msg, len);
        error_buffer[len] = '\0';
        error_head.next = e->next;
        free(e);
        return error_buffer;
    }
    return 0;
}
