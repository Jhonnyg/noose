#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "noose.h"

namespace app
{
    struct command
    {
        enum command_id
        {
            NO_COMMAND = 0,
            VERIFY_CPU,
            PRINT_HEADER
        } id;

        struct payload
        {
            char verify_log_path[256];
        } data;

        command* next;
    };

    command* make_command(command::command_id id, command* next)
    {
        command* cmd = (command*) malloc(sizeof(command));
        memset(cmd, 0, sizeof(*cmd));
        cmd->id = id;
        cmd->next = next;
        return cmd;
    }

    command* get_commands(int argc, char const *argv[])
    {
        command* last_cmd = make_command(command::NO_COMMAND, 0);

        for (int i = 1; i < argc; ++i)
        {
            const char* arg = argv[i];

            if (arg[0] == '-')
            {
                if (strcmp(arg, "-verify") == 0)
                {
                    last_cmd = make_command(command::VERIFY_CPU, last_cmd);
                    const char* verify_path = argv[i+1];
                    size_t verify_path_len = strlen(verify_path);
                    memcpy(last_cmd->data.verify_log_path, verify_path, verify_path_len);
                }
                else if (strcmp(arg, "-print_header") == 0)
                {
                    last_cmd = make_command(command::PRINT_HEADER, last_cmd);
                }
            }
        }

        return last_cmd;
    }

    void delete_commands(command* cmd)
    {
        command* c = cmd;
        while(c->next)
        {
            command* nxt = c->next;
            free(c);
            c = nxt;
        }
    }

    void process_commands(command* cmd, noose::rom rom)
    {
        command* it = cmd;
        while(it)
        {
            switch(it->id)
            {
                case command::VERIFY_CPU:
                    noose::debug("CMD :: Verifying ROM");
                    if (!noose::verify_rom(&rom, cmd->data.verify_log_path))
                    {
                        noose::error("Verification failed, reason:");
                        while(noose::has_errors())
                        {
                            noose::error(noose::last_error());
                        }
                    }
                    break;
                case command::PRINT_HEADER:
                    noose::debug("CMD :: Print Header");
                    noose::print_header(rom.header);
                    break;
                default:break;
            }

            it = it->next;
        }
    }
}

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
        while(noose::has_errors())
        {
            noose::error(noose::last_error());
        }
    }

    app::command* cmd = app::get_commands(argc, argv);

    app::process_commands(cmd, rom);

    noose::reset_rom(&rom);

    app::delete_commands(cmd);

    return 0;
}
