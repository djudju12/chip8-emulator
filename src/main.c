#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <raylib.h>

#define ASSERT assert
// each pixel in the frame buffer will map to WINDOW_FACTOR in the pc
// running the emulator
#define WINDOW_FACTOR 10

#define TODO(msg) ASSERT(0 && msg)

// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
// 3.1 - Standard Chip-8 Instructions
// 00E0 - CLS
// 00EE - RET
// 0nnn - SYS addr
// 2nnn - CALL addr
// 3xkk - SE Vx, byte
// 5xy0 - SE Vx, Vy
// 8xy1 - OR Vx, Vy
// 8xy2 - AND Vx, Vy
// 8xy3 - XOR Vx, Vy
// 8xy5 - SUB Vx, Vy
// 8xy6 - SHR Vx {, Vy}
// 8xy7 - SUBN Vx, Vy
// 8xyE - SHL Vx {, Vy}
// 4xkk - SNE Vx, byte
// 9xy0 - SNE Vx, Vy
// 1nnn - JP addr
// Bnnn - JP V0, addr
// Cxkk - RND Vx, byte
// Dxyn - DRW Vx, Vy, nibble
// Ex9E - SKP Vx
// ExA1 - SKNP Vx
// 7xkk - ADD Vx, byte
// 8xy4 - ADD Vx, Vy
// Fx1E - ADD I, Vx
// 6xkk - LD Vx, byte
// 8xy0 - LD Vx, Vy
// Annn - LD I, addr
// Fx07 - LD Vx, DT
// Fx0A - LD Vx, K
// Fx15 - LD DT, Vx
// Fx18 - LD ST, Vx
// Fx29 - LD F, Vx
// Fx33 - LD B, Vx
// Fx55 - LD [I], Vx
// Fx65 - LD Vx, [I]

typedef enum {
    OP_CLS = 0    ,
    OP_RET        ,
    OP_SYS        ,
    OP_CALL       ,
    OP_SE_RB      ,
    OP_SE_RR      ,
    OP_OR         ,
    OP_AND        ,
    OP_XOR        ,
    OP_SUB        ,
    OP_SHR        ,
    OP_SUBN       ,
    OP_SHL        ,
    OP_SNE_R_B    ,
    OP_SNE_R_R    ,
    OP_JP_ADDR    ,
    OP_JP_V0_ADDR ,
    OP_RND        ,
    OP_DRW        ,
    OP_SKP        ,
    OP_SKNP       ,
    OP_ADD_R_B    ,
    OP_ADD_R_R    ,
    OP_ADD_I_R    ,
    OP_LD_R_B     ,
    OP_LD_R_R     ,
    OP_LD_I_ADDR  ,
    OP_LD_R_DT    ,
    OP_LD_R_K     ,
    OP_LD_DT_R    ,
    OP_LD_ST_R    ,
    OP_LD_FONT_R  ,
    OP_LD_BCD_R   ,
    OP_LD_IMEM_R  ,
    OP_LD_R_IMEM  ,
    __OP_CNT__
} Op_Type;

typedef uint16_t Op;

typedef struct {
    uint16_t mask, value;
} Op_Pattern;

static Op_Pattern op_decode_table[__OP_CNT__] = {
    [OP_CLS]         = { 0xFFFF, 0x00E0 },
    [OP_RET]         = { 0xFFFF, 0x00EE },
    [OP_SYS]         = { 0xF000, 0x0000 },
    [OP_CALL]        = { 0xF000, 0x2000 },
    [OP_SE_RB]       = { 0xF000, 0x3000 },
    [OP_SE_RR]       = { 0xF000, 0x5000 },
    [OP_OR]          = { 0xF00F, 0x8001 },
    [OP_AND]         = { 0xF00F, 0x8002 },
    [OP_XOR]         = { 0xF00F, 0x8003 },
    [OP_SUB]         = { 0xF00F, 0x8005 },
    [OP_SHR]         = { 0xF00F, 0x8006 },
    [OP_SUBN]        = { 0xF00F, 0x8007 },
    [OP_SHL]         = { 0xF00F, 0x800E },
    [OP_SNE_R_B]     = { 0xF000, 0x4000 },
    [OP_SNE_R_R]     = { 0xF00F, 0x9000 },
    [OP_JP_ADDR]     = { 0xF000, 0x1000 },
    [OP_JP_V0_ADDR]  = { 0xF000, 0xB000 },
    [OP_RND]         = { 0xF000, 0xC000 },
    [OP_DRW]         = { 0xF000, 0xD000 },
    [OP_SKP]         = { 0xF0FF, 0xE09E },
    [OP_SKNP]        = { 0xF0FF, 0xE0A1 },
    [OP_ADD_R_B]     = { 0xF000, 0x7000 },
    [OP_ADD_R_R]     = { 0xF00F, 0x8004 },
    [OP_ADD_I_R]     = { 0xF0FF, 0xF01E },
    [OP_LD_R_B]      = { 0xF000, 0x6000 },
    [OP_LD_R_R]      = { 0xF00F, 0x8000 },
    [OP_LD_I_ADDR]   = { 0xF000, 0xA000 },
    [OP_LD_R_DT]     = { 0xF0FF, 0xF007 },
    [OP_LD_R_K]      = { 0xF0FF, 0xF00A },
    [OP_LD_DT_R]     = { 0xF0FF, 0xF015 },
    [OP_LD_ST_R]     = { 0xF0FF, 0xF018 },
    [OP_LD_FONT_R]   = { 0xF0FF, 0xF029 },
    [OP_LD_BCD_R]    = { 0xF0FF, 0xF033 },
    [OP_LD_IMEM_R]   = { 0xF0FF, 0xF055 },
    [OP_LD_R_IMEM]   = { 0xF0FF, 0xF065 }
};

static char *op_names[__OP_CNT__] = {
    [OP_CLS]         = "OP_CLS",
    [OP_RET]         = "OP_RET",
    [OP_SYS]         = "OP_SYS",
    [OP_CALL]        = "OP_CALL",
    [OP_SE_RB]       = "OP_SE_RB",
    [OP_SE_RR]       = "OP_SE_RR",
    [OP_OR]          = "OP_OR",
    [OP_AND]         = "OP_AND",
    [OP_XOR]         = "OP_XOR",
    [OP_SUB]         = "OP_SUB",
    [OP_SHR]         = "OP_SHR",
    [OP_SUBN]        = "OP_SUBN",
    [OP_SHL]         = "OP_SHL",
    [OP_SNE_R_B]     = "OP_SNE_R_B",
    [OP_SNE_R_R]     = "OP_SNE_R_R",
    [OP_JP_ADDR]     = "OP_JP_ADDR",
    [OP_JP_V0_ADDR]  = "OP_JP_V0_ADDR",
    [OP_RND]         = "OP_RND",
    [OP_DRW]         = "OP_DRW",
    [OP_SKP]         = "OP_SKP",
    [OP_SKNP]        = "OP_SKNP",
    [OP_ADD_R_B]     = "OP_ADD_R_B",
    [OP_ADD_R_R]     = "OP_ADD_R_R",
    [OP_ADD_I_R]     = "OP_ADD_I_R",
    [OP_LD_R_B]      = "OP_LD_R_B",
    [OP_LD_R_R]      = "OP_LD_R_R",
    [OP_LD_I_ADDR]   = "OP_LD_I_ADDR",
    [OP_LD_R_DT]     = "OP_LD_R_DT",
    [OP_LD_R_K]      = "OP_LD_R_K",
    [OP_LD_DT_R]     = "OP_LD_DT_R",
    [OP_LD_ST_R]     = "OP_LD_ST_R",
    [OP_LD_FONT_R]   = "OP_LD_FONT_R",
    [OP_LD_BCD_R]    = "OP_LD_BCD_R",
    [OP_LD_IMEM_R]   = "OP_LD_IMEM_R",
    [OP_LD_R_IMEM]   = "OP_LD_R_IMEM"
};

// - The sound and delay timers sequentially decrease at a rate of 1 per tick of a 60Hz clock. When the
// sound timer is above 0, the sound will play as a single monotone beep.

// - The framebuffer is an (x, y) addressable memory array that designates whether a pixel is currently on
// or off. This will be implemented with a write address, an (x, y) position, a offset in the x direction,
// and an 8-bit group of pixels to be drawn to the screen.

// - The return address stack stores previous program counters when jumping into a new routine.

// - The VF register is frequently used for storing carry values from a subtraction or addition action, and
// also specifies whether a particular pixel is to be drawn on the screen.

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 0x10
#define FRAME_W 64
#define FRAME_H 32
#define FRAME_BUFFER_SIZE FRAME_H*FRAME_W

enum {
    CHIP8_KEY_1 = 0b1000000000000000,
    CHIP8_KEY_2 = 0b0100000000000000,
    CHIP8_KEY_3 = 0b0010000000000000,
    CHIP8_KEY_C = 0b0001000000000000,
    CHIP8_KEY_4 = 0b0000100000000000,
    CHIP8_KEY_5 = 0b0000010000000000,
    CHIP8_KEY_6 = 0b0000001000000000,
    CHIP8_KEY_D = 0b0000000100000000,
    CHIP8_KEY_7 = 0b0000000010000000,
    CHIP8_KEY_8 = 0b0000000001000000,
    CHIP8_KEY_9 = 0b0000000000100000,
    CHIP8_KEY_E = 0b0000000000010000,
    CHIP8_KEY_A = 0b0000000000001000,
    CHIP8_KEY_0 = 0b0000000000000100,
    CHIP8_KEY_B = 0b0000000000000010,
    CHIP8_KEY_F = 0b0000000000000001
};

static struct { uint16_t chip8; int raylib; }
keyboard_decode_table[0x10] = {
    [0x1] = { .chip8 = CHIP8_KEY_1 , .raylib = KEY_ONE    } ,
    [0x2] = { .chip8 = CHIP8_KEY_2 , .raylib = KEY_TWO    } ,
    [0x3] = { .chip8 = CHIP8_KEY_3 , .raylib = KEY_THREE  } ,
    [0xC] = { .chip8 = CHIP8_KEY_C , .raylib = KEY_C      } ,
    [0x4] = { .chip8 = CHIP8_KEY_4 , .raylib = KEY_FOUR   } ,
    [0x5] = { .chip8 = CHIP8_KEY_5 , .raylib = KEY_FIVE   } ,
    [0x6] = { .chip8 = CHIP8_KEY_6 , .raylib = KEY_SIX    } ,
    [0xD] = { .chip8 = CHIP8_KEY_D , .raylib = KEY_D      } ,
    [0x7] = { .chip8 = CHIP8_KEY_7 , .raylib = KEY_SEVEN  } ,
    [0x8] = { .chip8 = CHIP8_KEY_8 , .raylib = KEY_EIGHT  } ,
    [0x9] = { .chip8 = CHIP8_KEY_9 , .raylib = KEY_NINE   } ,
    [0xE] = { .chip8 = CHIP8_KEY_E , .raylib = KEY_E      } ,
    [0xA] = { .chip8 = CHIP8_KEY_A , .raylib = KEY_A      } ,
    [0x0] = { .chip8 = CHIP8_KEY_0 , .raylib = KEY_ZERO   } ,
    [0xB] = { .chip8 = CHIP8_KEY_B , .raylib = KEY_B      } ,
    [0xF] = { .chip8 = CHIP8_KEY_F , .raylib = KEY_F      } ,
};

typedef struct {
    uint64_t frame_buffer[FRAME_H];
    int8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t memory[MEMORY_SIZE];
    uint8_t regs[0x10];
    uint16_t stack[STACK_SIZE];
    uint16_t pc;
    uint16_t regi;
    uint16_t keyboard;
} Chip8;

bool read_rom_to_memory(Chip8 *chip8, const char *rom) {
    bool status = true;
    FILE *file = fopen(rom, "rb");
    if (file == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %s\n", rom, strerror(errno));
        status = false;
        goto ERROR;
    }

    long size;
    if (fseek(file, 0, SEEK_END) < 0 || (size = ftell(file)) < 0) {
        fprintf(stderr, "ERROR: something went wrong in the reading of %s: %s\n", rom, strerror(errno));
        status = false;
        goto ERROR;
    }

    if (size >= MEMORY_SIZE - 0x200) {
        fprintf(stderr, "ERROR: rom %s is too big. The specified size is %d and the program must start at 0x200\n", rom, MEMORY_SIZE);
        status = false;
        goto ERROR;
    }

    rewind(file);
    if ((long) fread(chip8->memory + 0x200, sizeof(*chip8->memory)*size, size, file) == 0) {
        fprintf(stderr, "ERROR: could not read entire file %s: %s\n", rom, strerror(errno));
        status = false;
        goto ERROR;
    }

ERROR:
    if (file) {
        fclose(file);
    }

    return status;
}

Op op_fetch(Chip8 chip8) {
    uint16_t addr = chip8.pc;
    if (addr > MEMORY_SIZE - 1) {
        fprintf(stderr, "ERROR: trying to accessing a out of bound adress (%d)\n", addr);
        exit(1);
    }

    return chip8.memory[addr] << 8 | chip8.memory[addr + 1];
}

Op_Type op_decode(Op op) {
    for (Op_Type type = 0; type < __OP_CNT__; type++) {
        Op_Pattern op_pattern = op_decode_table[type];
        if ((op_pattern.mask & op) == op_pattern.value) {
            return type;
        }
    }

    assert(0 && "unreacheable");
}

bool is_pixel_active(Chip8 chip8, int x, int y) {
    return ((chip8.frame_buffer[y] << x) & ((uint64_t) 0x1 << 63)) != 0;
}

void blit_frame_buffer(Chip8 chip8) {
    for (uint8_t y = 0; y < FRAME_H; y++) {
        for (uint8_t x = 0; x < FRAME_W; x++) {
            uint8_t mx = FRAME_W - 1 - x;
            if (is_pixel_active(chip8, x, y)) {
                DrawRectangle(mx*WINDOW_FACTOR, y*WINDOW_FACTOR, WINDOW_FACTOR, WINDOW_FACTOR, WHITE);
            } else {
                DrawRectangle(mx*WINDOW_FACTOR, y*WINDOW_FACTOR, WINDOW_FACTOR, WINDOW_FACTOR, BLACK);
            }
        }
    }
}

void chip8_dump(Chip8 chip8) {
    for (int i = 0; i < MEMORY_SIZE;) {
        printf("0x%04x: ", i);
        for (int k = 0; i < MEMORY_SIZE && k < 8; k++, i += 2) {
            uint16_t x = chip8.memory[i] << 8 | chip8.memory[i + 1];
            printf("%04x ", x);
        }

        printf("\n");
    }
}

char *shift(int *argc, char ***argv) {
    return (*argc)--, *(*argv)++;
}

int main(int argc, char **argv) {
    char *program_name = shift(&argc, &argv);
    if (argc <= 0) {
        fprintf(stderr, "ERROR: missing ROM file\n");
        printf("    usage: %s <ROM.ch8>\n", program_name);
        return 1;
    }


    Chip8 chip8 = {0};
    char *rom = shift(&argc, &argv);
    if (!read_rom_to_memory(&chip8, rom)) {
        return 1;
    }

    chip8.pc = 0x200;

#if defined(DUMP_AND_DIE)
    chip8_dump(chip8);
    return 0;
#endif

#if defined(DEBUG)
    chip8_dump(chip8);
#endif

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(FRAME_W*WINDOW_FACTOR, FRAME_H*WINDOW_FACTOR, "Chip8");

    Op op;
    Op_Type type;
    float dt = 0.0;
    bool waiting_for_key = false;
    while (!WindowShouldClose()) {
        dt += GetFrameTime();
        if (dt >= 1/60.0) {
            dt = 0;
            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) chip8.sound_timer--;
        }

        for (int i = 0; i < 16; i++) {
            if (IsKeyDown(keyboard_decode_table[i].raylib)) {
                chip8.keyboard |= keyboard_decode_table[i].chip8;
            }

            if ( chip8.keyboard & keyboard_decode_table[i].chip8 &&
                 IsKeyUp(keyboard_decode_table[i].raylib)
            ) {
                chip8.keyboard &= ~keyboard_decode_table[i].chip8;

                if (waiting_for_key) {
                    waiting_for_key = false;
                    uint8_t x = (op & 0x0F00) >> 8;
                    chip8.regs[x] = i;
                }
            }
        }

        if (!waiting_for_key) {
            op = op_fetch(chip8);
            type = op_decode(op);

#if defined(DEBUG)
            printf("0x%04x: 0x%04x | DECODED: %s [%d]\n", chip8.pc, op, op_names[type], type);
#endif

            switch (type) {
                // 00E0 - CLS
                case OP_CLS: {
                    memset(chip8.frame_buffer, 0, sizeof(*chip8.frame_buffer)*FRAME_H);
                    chip8.pc += 2;
                } break;

                // 00EE - RET
                case OP_RET: {
                    if (chip8.sp <= 0) {
                        fprintf(stderr, "ERROR: stack underflow\n");
                        return 1;
                    }

                    chip8.pc = chip8.stack[--chip8.sp];
                } break;

                // 2nnn - CALL addr
                case OP_CALL: {
                    if (chip8.sp >= STACK_SIZE) {
                        fprintf(stderr, "ERROR: stack overflow\n");
                        return 1;
                    }

                    chip8.stack[chip8.sp++] = chip8.pc + 2;
                    chip8.pc = op & 0x0FFF;
                } break;

                // 0nnn - SYS addr
                case OP_SYS: {
                    TODO("OP_SYS");
                } break;

                // 3xkk - SE Vx, byte
                case OP_SE_RB: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t k = op & 0x00FF;
                    if (chip8.regs[x] == k) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // 5xy0 - SE Vx, Vy
                case OP_SE_RR: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    if (chip8.regs[x] == chip8.regs[y]) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // 8xy1 - OR Vx, Vy
                case OP_OR: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    chip8.regs[x] |= chip8.regs[y];
                    chip8.regs[0xF] = 0;
                    chip8.pc += 2;
                } break;

                // 8xy2 - AND Vx, Vy
                case OP_AND: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    chip8.regs[x] &= chip8.regs[y];
                    chip8.regs[0xF] = 0;
                    chip8.pc += 2;
                } break;

                // 8xy3 - XOR Vx, Vy
                case OP_XOR: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    chip8.regs[x] ^= chip8.regs[y];
                    chip8.regs[0xF] = 0;
                    chip8.pc += 2;
                } break;

                // 8xy5 - SUB Vx, Vy
                case OP_SUB: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    uint8_t vx = chip8.regs[x];
                    uint8_t vy = chip8.regs[y];

                    chip8.regs[x] = vx - vy;
                    chip8.regs[0xF] = vx >= vy;
                    chip8.pc += 2;
                } break;

                // 8xy6 - SHR Vx {, Vy}
                case OP_SHR: {
                    // I found very strange that we accept VY but dont use it
                    // Its actually a quirk -> https://chip8.gulrak.net/#quirk6
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    uint8_t vy = chip8.regs[y];
                    chip8.regs[x] = vy >> 1;
                    chip8.regs[0xF] = vy & 1;
                    chip8.pc += 2;
                } break;

                // 8xyE - SHL Vx {, Vy}
                case OP_SHL: {
                    // I found very strange that we accept VY but dont use it
                    // Its actually a quirk -> https://chip8.gulrak.net/#quirk6
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    uint8_t vy = chip8.regs[y];
                    chip8.regs[x] = vy << 1;
                    chip8.regs[0xF] = (vy >> 7) & 1;
                    chip8.pc += 2;
                } break;

                // 8xy7 - SUBN Vx, Vy
                case OP_SUBN: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    uint8_t vx = chip8.regs[x];
                    uint8_t vy = chip8.regs[y];

                    chip8.regs[x] = vy - vx;
                    chip8.regs[0xF] = vy >= vx;
                    chip8.pc += 2;
                } break;

                // 4xkk - SNE Vx, byte
                case OP_SNE_R_B: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t k = op & 0x0FF;
                    if (chip8.regs[x] != k) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // 9xy0 - SNE Vx, Vy
                case OP_SNE_R_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    if (chip8.regs[x] != chip8.regs[y]) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // 1nnn - JP addr
                case OP_JP_ADDR: {
                    chip8.pc = op & 0x0FFF;
                } break;

                // Bnnn - JP V0, addr
                case OP_JP_V0_ADDR: {
                    uint16_t addr = op & 0x0FFF;
                    chip8.pc = addr + chip8.regs[0];
                } break;

                // Cxkk - RND Vx, byte
                case OP_RND: {
                    TODO("OP_RND");
                } break;

                // Dxyn - DRW Vx, Vy, nibble
                case OP_DRW: {
                    chip8.regs[0xF] = 0;
                    int8_t x = chip8.regs[(op & 0x0F00) >> 8] % FRAME_W;
                    uint8_t y = chip8.regs[(op & 0x00F0) >> 4] % FRAME_H;
                    uint8_t n = op & 0x000F;
                    for (int8_t i = 0; y < FRAME_H && i < n; i++, y++) {
                        uint16_t mem = chip8.regi + i;
                        if (mem >= MEMORY_SIZE) {
                            fprintf(stderr, "ERROR: out of bounds access\n");
                            return 1;
                        }

                        uint8_t sprite_byte = chip8.memory[mem];
                        for (uint8_t k = 0, index = x + k; index < FRAME_W && k < 8; k++, index = x + k) {
                            uint8_t current_bit = (chip8.frame_buffer[y] >> index) & 1; // Get current state of pixel
                            uint8_t sprite_bit = (sprite_byte >> (7 - k)) & 1;          // Get state from sprite
                            uint64_t frame_bit = sprite_bit^current_bit;                // Get new pixel state
                            chip8.frame_buffer[y] &= ~((uint64_t)1 << index);           // Clear the current pixel
                            chip8.frame_buffer[y] |= frame_bit << index;                // Set bit on the frame buffer

                            chip8.regs[0xF] |= current_bit & sprite_bit;
                        }
                    }

                    chip8.pc += 2;
                } break;

                // Ex9E - SKP Vx
                case OP_SKP: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t key = chip8.regs[x];
                    if (key > 0xF) {
                        fprintf(stderr, "ERROR: invalid key %d\n", key);
                        return 1;
                    }

                    if (chip8.keyboard & keyboard_decode_table[key].chip8) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // ExA1 - SKNP Vx
                case OP_SKNP: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t key = chip8.regs[x];
                    if (key > 0xF) {
                        fprintf(stderr, "ERROR: invalid key %d\n", key);
                        return 1;
                    }

                    if (!(chip8.keyboard & keyboard_decode_table[key].chip8)) {
                        chip8.pc += 2;
                    }

                    chip8.pc += 2;
                } break;

                // 7xkk - ADD Vx, byte
                case OP_ADD_R_B: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t value = op & 0x00FF;
                    chip8.regs[x] = (chip8.regs[x] + value) & 0xFF;
                    chip8.pc += 2;
                } break;

                // 8xy4 - ADD Vx, Vy
                case OP_ADD_R_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    uint16_t t = chip8.regs[x] + chip8.regs[y];
                    chip8.regs[x] = t;
                    chip8.regs[0xF] = t > 255;
                    chip8.pc += 2;
                } break;

                // Fx1E - ADD I, Vx
                case OP_ADD_I_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    chip8.regi += chip8.regs[x];
                    chip8.pc += 2;
                } break;

                // 6xkk - LD Vx, byte
                case OP_LD_R_B: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    chip8.regs[x] = op & 0x00FF;
                    chip8.pc += 2;
                } break;

                // 8xy0 - LD Vx, Vy
                case OP_LD_R_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint8_t y = (op & 0x00F0) >> 4;
                    chip8.regs[x] = chip8.regs[y];
                    chip8.pc += 2;
                } break;

                // Annn - LD I, addr
                case OP_LD_I_ADDR: {
                    chip8.regi = op & 0x0FFF;
                    chip8.pc += 2;
                } break;

                // Fx07 - LD Vx, DT
                case OP_LD_R_DT: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    chip8.regs[x] = chip8.delay_timer;
                    chip8.pc += 2;
                } break;

                // Fx0A - LD Vx, K
                case OP_LD_R_K: {
                    waiting_for_key = true;
                    chip8.pc += 2;
                } break;

                // Fx15 - LD DT, Vx
                case OP_LD_DT_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    chip8.delay_timer = chip8.regs[x];
                    chip8.pc += 2;
                } break;

                // Fx18 - LD ST, Vx
                case OP_LD_ST_R: {
                    TODO("OP_LD_ST_R");
                } break;

                // Fx29 - LD F, Vx
                case OP_LD_FONT_R: {
                    TODO("OP_LD_FONT_R");
                } break;

                // Fx33 - LD B, Vx
                case OP_LD_BCD_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    uint16_t start = chip8.regi;
                    if (start >= (MEMORY_SIZE - 3)) {
                        printf("ERROR: OP_LD_BCD_R out of bounds\n");
                        return 1;
                    }

                    uint8_t v = chip8.regs[x];
                    chip8.memory[start + 0] = v / 100;
                    chip8.memory[start + 1] = (v / 10) % 10;
                    chip8.memory[start + 2] = (v % 10) % 10;
                    chip8.pc += 2;
                } break;

                // Fx55 - LD [I], Vx
                case OP_LD_IMEM_R: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; i++) {
                        uint16_t mem = chip8.regi++;
                        if (mem >= MEMORY_SIZE) {
                            printf("ERROR: OP_LD_R_IMEM out of bounds\n");
                            return 1;
                        }

                        chip8.memory[mem] = chip8.regs[i];
                    }

                    chip8.pc += 2;
                } break;

                // Fx65 - LD Vx, [I]
                case OP_LD_R_IMEM: {
                    uint8_t x = (op & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; i++) {
                        uint16_t mem = chip8.regi++;
                        if (mem >= MEMORY_SIZE) {
                            printf("ERROR: OP_LD_R_IMEM out of bounds\n");
                            return 1;
                        }

                        chip8.regs[i] = chip8.memory[mem];
                    }

                    chip8.pc += 2;
                } break;

                default: {
                    fprintf(stderr, "ERROR: op %04x not implemented", op);
                    return 1;
                }
            }
        }

        BeginDrawing();
        blit_frame_buffer(chip8);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}