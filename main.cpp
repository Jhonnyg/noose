#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noose.h"

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        noose::error("Invalid number of arguments.");
        noose::print_help();
        return -1;
    }

    noose::rom rom = {};
    if (!noose::load_rom(argv[1], &rom))
    {
        noose::error("Unable to load rom, reason:");
        noose::error(noose::last_error());
    }

    noose::print_header(rom.header);
    noose::reset_rom(&rom);

    return 0;
}
