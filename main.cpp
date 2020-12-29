#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
0-3: Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
4: Size of PRG ROM in 16 KB units
5: Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
6: Flags 6 - Mapper, mirroring, battery, trainer
7: Flags 7 - Mapper, VS/Playchoice, NES 2.0
8: Flags 8 - PRG-RAM size (rarely used extension)
9: Flags 9 - TV system (rarely used extension)
10: Flags 10 - TV system, PRG-RAM presence (unofficial, rarely used extension)
11-15: Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)
*/

// FLags 6
#define rom_header_nametable_mirroring_mode(h) (h.flags_6 & 0x01 >> 0)
#define rom_header_battery_backed_prg(h)       (h.flags_6 & 0x02 >> 1)
#define rom_header_has_trainer_data(h)         (h.flags_6 & 0x04 >> 2)
#define rom_header_ignore_mirror_control(h)    (h.flags_6 & 0x08 >> 3)
#define rom_header_mapper_number_lower(h)      (h.flags_6 & 0xf0 >> 4)
// Flags 7
#define rom_header_vs_unisystem(h)             (h.flags_7 & 0x01 >> 0)
#define rom_header_playchoice_10(h)            (h.flags_7 & 0x02 >> 1)
#define rom_header_nes_2_0_bits(h)             (h.flags_7 & 0x0c >> 2)
#define rom_header_mapper_number_higher(h)     (h.flags_7 & 0xf0 >> 4)
// Flags 8
#define rom_header_prg_ram_size(h)             (h.flags_8)
// Flags 9
#define rom_header_tv_system_1(h)              (h.flags_9 & 0x01)
// Flags 10
#define rom_header_tv_system_2(h)              (h.flags_10 & 0x03)
#define rom_header_prg_ram(h)                  (h.flags_10 & 0x10 >> 4)
#define rom_header_bus_conflict(h)             (h.flags_10 & 0x20 >> 5)

struct s_rom_header
{
    char    magic[4];
    uint8_t page_count_prg;
    uint8_t page_count_chr;
    uint8_t flags_6;
    uint8_t flags_7;
    uint8_t flags_8;
    uint8_t flags_9;
    uint8_t flags_10;
    uint8_t unused[5];
};

struct s_rom
{
    s_rom_header header;
    uint8_t      data_trainer[512];
    uint8_t*     data_prg;
    uint8_t*     data_chr;
    uint8_t      mapper_id;
};

typedef struct s_rom rom;
typedef struct s_rom_header rom_header;

bool has_magic_number(rom_header header)
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

void print_header(rom_header header)
{
    const char* tv_system_lut[] = {"NTCS", "PAL", "Dual Compatible"};

    printf("Magic                          : %s\n", header.magic);
    printf("Page count (PRG)               : %d\n", header.page_count_prg);
    printf("Page count (CHR)               : %d\n", header.page_count_chr);

    // Flags 6
    printf("Flag 6 (Nametable Mirror Mode) : %s\n", rom_header_nametable_mirroring_mode(header) ? "Vertical" : "Horizontal");
    printf("Flag 6 (Battery Backed PRG)    : %s\n", rom_header_battery_backed_prg(header) ? "True" : "False");
    printf("Flag 6 (Has Trainer Data)      : %s\n", rom_header_has_trainer_data(header) ? "True" : "False");
    printf("Flag 6 (Ignore Mirror Control) : %s\n", rom_header_ignore_mirror_control(header) ? "Ignore" : "Four-screen VRAM");
    printf("Flag 6 (Mapper Number Lower)   : %d\n", rom_header_mapper_number_lower(header));
    // Flags 7
    printf("Flag 7 (VS Unisystem)          : %s\n", rom_header_vs_unisystem(header) ? "True" : "False");
    printf("Flag 7 (Playchoice 10)         : %s\n", rom_header_playchoice_10(header) ? "True" : "False");
    printf("Flag 7 (NES 2.0)               : %s\n", rom_header_nes_2_0_bits(header) ? "True" : "False");
    printf("Flag 7 (Mapper Number Higher)  : %d\n", rom_header_mapper_number_higher(header));
    // Flags 8
    printf("Flag 8 (PRG RAM Size)          : %d\n", rom_header_prg_ram_size(header));
    // Flags 9
    printf("Flag 9 (TV System)             : %s\n", rom_header_tv_system_1(header) ? "NTCS" : "PAL");
    // Flags 10
    printf("Flag 10 (TV System)            : %s\n", tv_system_lut[rom_header_tv_system_2(header)]);
    printf("Flag 10 (PRG RAM)              : %s\n", rom_header_prg_ram(header) ? "True" : "False");
    printf("Flag 10 (Bus Conflict)         : %s\n", rom_header_bus_conflict(header) ? "Has Conflict" : "No Conflict");
}

rom load_rom(const char* path)
{
    FILE* f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    rewind(f);
    uint8_t* buffer = (uint8_t*) malloc(f_size);
    fread(buffer, sizeof(uint8_t), f_size, f);
    fclose(f);

    rom the_rom = {};
    memcpy(&the_rom.header, buffer, sizeof(rom_header));

    if (!has_magic_number(the_rom.header))
    {
        printf("Invalid header");
        return the_rom;
    }

    print_header(the_rom.header);

    uint32_t cursor = sizeof(rom_header);

    const uint32_t prg_block_size = 16384;
    const uint32_t chr_block_size = 8192;

    if (rom_header_has_trainer_data(the_rom.header))
    {
        uint32_t data_trainer_size = sizeof(the_rom.data_trainer);
        memcpy(the_rom.data_trainer, &buffer[cursor], data_trainer_size);
        cursor += data_trainer_size;
    }

    if (the_rom.header.page_count_prg > 0)
    {
        uint32_t bytes_to_alloc = prg_block_size * the_rom.header.page_count_prg;
        the_rom.data_prg = (uint8_t*) malloc(bytes_to_alloc);
        memcpy(the_rom.data_prg, &buffer[cursor], bytes_to_alloc);
        cursor += bytes_to_alloc;
    }

    if (the_rom.header.page_count_chr > 0)
    {
        uint32_t bytes_to_alloc = the_rom.header.page_count_chr * chr_block_size;
        the_rom.data_prg = (uint8_t*) malloc(bytes_to_alloc);
        memcpy(the_rom.data_chr, &buffer[cursor], bytes_to_alloc);
    }

    free(buffer);

    the_rom.mapper_id = rom_header_mapper_number_lower(the_rom.header) | (rom_header_mapper_number_higher(the_rom.header) << 4);

    return the_rom;
}

int main(int argc, char const *argv[])
{
    rom r = load_rom(argv[1]);

    return 0;
}
