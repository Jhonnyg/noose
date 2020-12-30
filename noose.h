#ifndef __NOOSE_H__
#define __NOOSE_H__

namespace noose
{
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
    struct s_header
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

        static uint8_t nametable_mirroring_mode(const s_header h) { return (h.flags_6 & 0x01 >> 0); }
        static uint8_t battery_backed_prg(const s_header h)       { return (h.flags_6 & 0x02 >> 1); }
        static uint8_t has_trainer_data(const s_header h)         { return (h.flags_6 & 0x04 >> 2); }
        static uint8_t ignore_mirror_control(const s_header h)    { return (h.flags_6 & 0x08 >> 3); }
        static uint8_t mapper_number_lower(const s_header h)      { return (h.flags_6 & 0xf0 >> 4); }
        static uint8_t vs_unisystem(const s_header h)             { return (h.flags_7 & 0x01 >> 0); }
        static uint8_t playchoice_10(const s_header h)            { return (h.flags_7 & 0x02 >> 1); }
        static uint8_t nes_2_0_bits(const s_header h)             { return (h.flags_7 & 0x0c >> 2); }
        static uint8_t mapper_number_higher(const s_header h)     { return (h.flags_7 & 0xf0 >> 4); }
        static uint8_t prg_ram_size(const s_header h)             { return (h.flags_8); }
        static uint8_t tv_system_1(const s_header h)              { return (h.flags_9 & 0x01); }
        static uint8_t tv_system_2(const s_header h)              { return (h.flags_10 & 0x03); }
        static uint8_t prg_ram(const s_header h)                  { return (h.flags_10 & 0x10 >> 4); }
        static uint8_t bus_conflict(const s_header h)             { return (h.flags_10 & 0x20 >> 5); }
    };

    struct s_rom
    {
        s_header header;
        uint8_t  data_trainer[512];
        uint8_t* data_prg;
        uint8_t* data_chr;
        uint8_t  mapper_id;
    };

    typedef struct s_rom    rom;
    typedef struct s_header header;

    bool load_rom(const char* path, rom* output);
    void reset_rom(noose::rom* rom);
    bool verify_rom(const noose::rom* rom, const char* verify_log_path);
    void debug(const char* debug_str);
    void error(const char* error_str);
    void print_help();
    void print_header(const header h);

    const char* last_error();
}

#endif /* __NOOSE_H__ */
