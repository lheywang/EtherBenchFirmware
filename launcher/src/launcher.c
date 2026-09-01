/**
 * @file    launcher.c
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Source file for the global launcher. Create and get the memory
 * 			for the different tasks.
 * @version 0.1
 * @date    2026-02-21
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "launcher.h"

// Local libraries
#include "main.h"

// ThreadX libraries
#include "app_filex.h"
#include "app_netxduo.h"
#include "app_threadx.h"
#include "app_usbx_device.h"
#include "tx_api.h"

// Tasks
#include "ShellStream/ShellStreamConfig.h"
#include "logger.h"
#include "task_leds.h"
#include "task_muxer.h"
#include "task_parser.h"
#include "task_programmer.h"

// STD
#include <stdint.h>

// ======================================================================
//                              MEMORY AREAS
// ======================================================================
/*
 * Leds (only simple blinking !) --> aRGB are handled a bit later.
 */
TX_THREAD leds_thread;
static __aligned(8) uint8_t leds_stack[IDLE_STACK_SIZE];

/*
 * Logger
 */
TX_THREAD logger_thread;
static __aligned(8) uint8_t logger_stack[LOGGER_STACK_SIZE];

/*
 * Parser
 */
TX_THREAD parser_thread;
static __aligned(8) uint8_t parser_stack[PARSER_STACK_SIZE];

TX_BLOCK_POOL parser_pool;
TX_QUEUE parser_input;
static __aligned(8) uint8_t parser_pool_mem[SHELL_POOL_TOTAL_SIZE];
static __aligned(8) ULONG parser_fifo_mem[SHELL_COMMAND_FIFO_DEPTH];

/*
 * Router
 */
TX_THREAD router_thread;
static __aligned(8) uint8_t router_stack[ROUTER_STACK_SIZE];

TX_BLOCK_POOL router_pool;
TX_BLOCK_POOL router_payloads;
TX_QUEUE router_input;
static __aligned(8) uint8_t router_payload_pool[ROUTER_PAYLOAD_POOL_SIZE];
static __aligned(8) uint8_t router_pool_mem[ROUTER_COMMAND_FIFO_SIZE];
static __aligned(8) ULONG router_fifo_mem[ROUTER_COMMAND_FIFO_DEPTH];

/*
 * Programmer
 */
TX_THREAD programmer_thread;
static __aligned(8) uint8_t programmer_stack[PROGRAMMER_STACK_SIZE];

TX_BLOCK_POOL programmer_pool;
TX_QUEUE programmer_input;
static __aligned(8) ULONG programmer_fifo_mem[CLIENT_FIFO_DEPTH];

/*
 * Sequencer
 */
TX_THREAD sequencer_thread;
static __aligned(8) uint8_t sequencer_stack[SEQUENCER_STACK_SIZE];

TX_BLOCK_POOL sequencer_pool;
TX_QUEUE sequencer_input;
static __aligned(8) ULONG sequencer_fifo_mem[CLIENT_FIFO_DEPTH];

/*
 * aRGB
 */
TX_THREAD argb_thread;
static __aligned(8) uint8_t argb_stack[ARGB_STACK_SIZE];

TX_BLOCK_POOL argb_pool;
TX_QUEUE argb_input;
static __aligned(8) ULONG argb_fifo_mem[CLIENT_FIFO_DEPTH];

/*
 * Hardware IOs
 */
TX_THREAD hardware_thread;
static __aligned(8) uint8_t hardware_stack[HARDWARE_STACK_SIZE];

TX_BLOCK_POOL hardware_pool;
TX_QUEUE hardware_input;
static __aligned(8) ULONG hardware_fifo_mem[CLIENT_FIFO_DEPTH];

/*
 * Board low level IO
 */
TX_THREAD board_thread;
static __aligned(8) uint8_t board_stack[BOARDCFG_STACK_SIZE];

TX_BLOCK_POOL board_pool;
TX_QUEUE board_input;
static __aligned(8) ULONG board_fifo_mem[CLIENT_FIFO_DEPTH];

/*
 * Semaphore
 */
TX_SEMAPHORE dma_trigger;
TX_SEMAPHORE dma_tx_done;

TX_SEMAPHORE flash_wip;
TX_SEMAPHORE flash_dma_done;

// ======================================================================
//                              FUNCTIONS
// ======================================================================

uint32_t launcher(void) {

    // -------------------------------------------------------------------
    // LOGGER
    // -------------------------------------------------------------------

    // Logger DMA tasks
    tx_semaphore_create(&dma_trigger, "dma trigger", 0);
    tx_semaphore_create(&dma_tx_done, "dma done", 0);

    /*
     * Creating the deferred logging task.
     * We do that first to ensure the messages WILL be deferred, when they could.
     */
    tx_thread_create(&logger_thread,
                     "Deferred Logger",
                     logger_task,
                     0,
                     logger_stack,
                     LOGGER_STACK_SIZE,
                     28,
                     28,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    // -------------------------------------------------------------------
    // SEMAPHORES
    // -------------------------------------------------------------------

    // FileX
    tx_semaphore_create(&flash_wip, "flash write in progress", 0);
    tx_semaphore_create(&flash_dma_done, "flash dma done", 0);

    // -------------------------------------------------------------------
    // MEMORY POOLS
    // -------------------------------------------------------------------

    // Parser
    tx_block_pool_create(
        &parser_pool, "Parser command Pool", SHELL_STRUCT_SIZE, &parser_pool_mem, sizeof(parser_pool_mem));
    tx_queue_create(&parser_input, "Parser command Queue", TX_1_ULONG, &parser_fifo_mem, sizeof(parser_fifo_mem));

    // Router
    tx_block_pool_create(&router_payloads,
                         "Router payload Pool",
                         ROUTER_TYPICAL_PAYLOAD,
                         &router_payload_pool,
                         sizeof(router_payload_pool));
    tx_block_pool_create(
        &router_pool, "Router command Pool", ROUTER_COMMAND_SIZE, &router_pool_mem, sizeof(router_pool_mem));
    tx_queue_create(&router_input, "Router Command Queue", TX_1_ULONG, &router_fifo_mem, sizeof(router_fifo_mem));

    // -------------------------------------------------------------------
    // THREADS
    // -------------------------------------------------------------------

    /*
     * Creating the router task
     */
    tx_thread_create(&router_thread,
                     "Router thread",
                     muxer_task,
                     0,
                     router_stack,
                     ROUTER_STACK_SIZE,
                     10,
                     10,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /*
     * Creating the parser task
     */
    tx_thread_create(&parser_thread,
                     "Parser",
                     parser_task,
                     0,
                     parser_stack,
                     PARSER_STACK_SIZE,
                     20,
                     20,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /*
     * Creating the leds task
     */
    tx_thread_create(&leds_thread,
                     "Leds control",
                     leds_task,
                     0,
                     leds_stack,
                     IDLE_STACK_SIZE,
                     28,
                     28,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    /*
     * Creating the leds task
     */
    tx_thread_create(&programmer_thread,
                     "Programmer",
                     programmer_task,
                     0,
                     programmer_stack,
                     PROGRAMMER_STACK_SIZE,
                     15,
                     15,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);

    // -------------------------------------------------------------------
    // LOW LEVEL IOS
    // -------------------------------------------------------------------

    /*
     * Launching the USBX task
     */
    MX_USBX_Device_Init();

    /*
     * Launch the NetXDuo task
     */
    MX_NetXDuo_Init();

    /*
     * Launch the FileX task
     */
    MX_FileX_Init();

    return 0;
}
