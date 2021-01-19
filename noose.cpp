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

/*
static void dbg_write_instruction_to_buffer(const cpu::instruction i, char* buffer, uint8_t op0, uint8_t op1)
{
    switch(i.address_mode)
    {
        case cpu::accumulator:
            sprintf(buffer, "accumulator");
            break;
        case cpu::absolute:
            sprintf(buffer, "absolute");
            break;
        case cpu::absolute_x_indexed:
            sprintf(buffer, "absolute_x_indexed");
            break;
        case cpu::absolute_y_indexed:
            sprintf(buffer, "absolute_y_indexed");
            break;
        case cpu::immediate:
            sprintf(buffer, "immediate");
            break;
        case cpu::implied:
            sprintf(buffer, "implied");
            break;
        case cpu::indirect:
            sprintf(buffer, "indirect");
            break;
        case cpu::x_indexed_indirect:
            sprintf(buffer, "x_indexed_indirect");
            break;
        case cpu::indirect_y_indexed:
            sprintf(buffer, "indirect_y_indexed");
            break;
        case cpu::relative:
            sprintf(buffer, "relative");
            break;
        case cpu::zeropage:
            sprintf(buffer, "zeropage");
            break;
        case cpu::zeropage_x_indexed:
            sprintf(buffer, "zeropage_x_indexed");
            break;
        case cpu::zeropage_y_indexed:
            sprintf(buffer, "zeropage_y_indexed");
            break;
        case cpu::unused:
            sprintf(buffer, "unused");
            break;
    }
}
*/

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

        noose::cpu::execute(next);

        // F5 C5  JMP $C5F5

        char instruction_str[40] = {};

        // dbg_write_instruction_to_buffer(next, buffer_noose, )

        sprintf(buffer_noose, "%X  %X %s %s  %-32sA:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%d\n",
            pc, next.code, "o0", "o1", instruction_str, reg_a, reg_x, reg_y, p, sp, ppu_x, ppu_y, cycle_count);

        #define COLOR_NRM  "\x1B[0m"
        #define COLOR_RED  "\x1B[31m"
        #define COLOR_GRN  "\x1B[32m"

        uint16_t cursor = 0;
        const char* last_color = COLOR_NRM;

        while(buffer_log[cursor] && cursor <= sizeof(buffer_log))
        {
            if (buffer_noose[cursor] == buffer_log[cursor])
            {
                if (last_color != COLOR_GRN)
                {
                    printf("%s", COLOR_GRN);
                    last_color = COLOR_GRN;
                }

                printf("%c", buffer_log[cursor]);
            }
            else
            {
                if (last_color != COLOR_RED)
                {
                    printf("%s", COLOR_RED);
                    last_color = COLOR_RED;
                }

                printf("%c", buffer_log[cursor]);
                abort = 1;
            }

            cursor++;
        }

        printf("%s\n", COLOR_NRM);

        if (abort)
        {
            add_error("Input string:");
            add_error(buffer_noose);
        }

        #undef COLOR_NRM
        #undef COLOR_RED
        #undef COLOR_GRN

        cycle_count += noose::cpu::get_instruction_meta(next).cycle_count;
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
