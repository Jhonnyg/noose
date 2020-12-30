#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noose.h"

static char error_buffer[512] = {};

static void set_last_error(const char* error_str)
{
    size_t error_str_size = strlen(error_str);
    assert(error_str_size < sizeof(error_buffer));
    memcpy(error_buffer, error_str, error_str_size);
    error_buffer[error_str_size] = '\0';
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
        set_last_error("Unable to open file");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    rewind(f);
    *buffer_out = (uint8_t*) malloc(f_size);
    *buffer_size = f_size;
    if (fread(*buffer_out, sizeof(uint8_t), f_size, f) != f_size)
    {
        set_last_error("Couldn't read all bytes in ROM");
        return true;
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
        set_last_error("Invalid header, no magic number");
        return false;
    }

    uint32_t cursor = sizeof(noose::header);

    const uint32_t prg_block_size = 16384;
    const uint32_t chr_block_size = 8192;

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
        output->data_prg = (uint8_t*) malloc(bytes_to_alloc);
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
