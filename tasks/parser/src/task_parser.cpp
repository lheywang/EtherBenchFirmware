/**
 * @file    task_logger.cpp
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Implement a logger module, which handle a fast,
 * 			DMA based IO, from high level functions.
 *
 * @version 0.1
 * @date    2026-03-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#define LOG_MODULE "PARSER"
// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "task_parser.h"

// Local libraries
#include "ShellStream/ShellStreamBase.hpp"
#include "commands/gperf.h"
#include "logger.h"
#include "task_shell.h"

// RTOS
#include "tx_api.h"

// STDLIB
#include <cstdio>
#include <cstring>
#include <ctype.h>

// ======================================================================
//                              VARIABLES
// ======================================================================
extern TX_BLOCK_POOL parser_pool;
extern TX_QUEUE parser_input;

// ======================================================================
//                              FUNCTIONS
// ======================================================================

void parser_task(ULONG arg) {
    TX_PARAMETER_NOT_USED(arg);

    parserInput_t *cmd = nullptr;
    char args[SHELL_LINE_LENGTH];

    while (1) {
        // Fetch the next message on the queue
        tx_queue_receive(&parser_input, &cmd, TX_WAIT_FOREVER);
        if (cmd == nullptr)
            continue;

        // Ensure we CAN respond to the command.
        if (cmd->reply_stream == nullptr)
            continue;
        ShellStreamBase *stream = static_cast<ShellStreamBase *>(cmd->reply_stream);

        /*
         * Normalize to lowercase the input command. This ensure the parsing WILL be done, regardless
         * of the standard. This is especially usefull as the standard SCPI commands are uppercase.
         */
        for (size_t i = 0; i < strlen(cmd->command); i++) {
            cmd->command[i] = tolower(cmd->command[i]);
        }

        /*
         * First, identify if the passed command is from SHELL or SCPI.
         * We consider that no shell command has '*', ':' or '?' in it. This is possible as we don't support
         * regex, which may include them. Do not reuse that method for any purpose without thinking.
         */
        char *ptr;
        int tokens[] = {'*', ':', '?'};
        int index = 0;
        bool isShell = true;

        do {
            ptr = strchr(cmd->command, tokens[index]);

            // if the char WAS found somewhere, we don't care where. But we know the command is SCPI.
            if ((ptr != NULL) && (ptr != cmd->command)) {
                isShell = false;
                break;
            }
            index += 1;

        } while (index < 3);

        /*
         * Search for a space in the command to mark the "split".
         * - If it exist, copy the remaining of the buffer to the args, which will be passed to the dedicated command
         *   handler.
         * - If not found, set the whole buffer to 0x00, which mark the end of the string.
         */
        int space = ' ';
        size_t command_size = 0;
        ptr = strchr(cmd->command, space);
        memset(args, 0x00, sizeof(args));

        if (ptr) {
            // Get the command size.
            command_size = (size_t)(ptr - cmd->command);

            // Copy the args to a safe buffer.
            const char *src_args = ptr + 1;
            size_t args_len = strlen(src_args);

            if (args_len >= sizeof(args)) {
                args_len = sizeof(args) - 1;
            }
            memcpy(args, src_args, args_len);

            // Set the null terminated strings to properly split the chains.
            args[args_len] = '\0';
            cmd->command[command_size] = '\0';

        } else {
            command_size = strlen(cmd->command);
        }

        /*
         * Parse the command as required. This is done using one of the matching gperf table.
         */
        const struct scpi_command_entry *parsed = nullptr;

        if (isShell) {
            parsed = find_shell_command(cmd->command, command_size);
        } else {
            parsed = find_scpi_command(cmd->command, command_size);
        }

        /*
         * Now, fetch the matching handler while trying to NOT go into an hardfault
         */
        if (parsed == nullptr) {
            LOG("Failed to parse the command. Ignoring this command.");
            continue;
        }

        /*
         * Execute the handler. We assume now that the major prints WILL be done by the command itself.
         */
        int retval = 0;
        if (parsed->handler != nullptr) {
            retval = parsed->handler(args, (void *)cmd->reply_stream);
            LOG("Executing command (%s) at 0x%08x", parsed->name, (uint32_t)parsed->handler);
        }

        // Inform the user of the fail, and let him cry. We don't have more informations than him.
        if (retval != 0) {
            stream->transmit("Command failed to execute with an error code : %d", retval);
        }

        // Free the command we received.
        tx_block_release(cmd);
    }
}
