#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SCALE 10
#define SCREEN_WIDTH 64 * SCALE
#define SCREEN_HEIGHT 32 * SCALE

struct chip8 {
    uint8_t *mem;            /* 4KB RAM */
    uint16_t pc;               /* Program counter */
    uint16_t *index;         /* Points the locaiton on memory */
    uint8_t deley_timer;     /* Decrements 60 per sec */
    uint8_t sound_timer;     /* Makes a beep sound if its bigger than 0 */
    uint8_t V[16];           /* general purpose variable from V0 to VF */
};

uint8_t font[][5] = {{0xF0, 0x90, 0x90, 0x90, 0xF0},  // 0
                     {0x20, 0x60, 0x20, 0x20, 0x70},  // 1
                     {0xF0, 0x10, 0xF0, 0x80, 0xF0},  // 2
                     {0xF0, 0x10, 0xF0, 0x10, 0xF0},  // 3
                     {0x90, 0x90, 0xF0, 0x10, 0x10},  // 4
                     {0xF0, 0x80, 0xF0, 0x10, 0xF0},  // 5
                     {0xF0, 0x80, 0xF0, 0x90, 0xF0},  // 6
                     {0xF0, 0x10, 0x20, 0x40, 0x40},  // 7
                     {0xF0, 0x90, 0xF0, 0x90, 0xF0},  // 8
                     {0xF0, 0x90, 0xF0, 0x10, 0xF0},  // 9
                     {0xF0, 0x90, 0xF0, 0x90, 0x90},  // A
                     {0xE0, 0x90, 0xE0, 0x90, 0xE0},  // B
                     {0xF0, 0x80, 0x80, 0x80, 0xF0},  // C
                     {0xE0, 0x90, 0x90, 0x90, 0xE0},  // D
                     {0xF0, 0x80, 0xF0, 0x80, 0xF0},  // E
                     {0xF0, 0x80, 0xF0, 0x80, 0x80}}; // F

struct chip8 *init_chip8() {
    struct chip8 *c8 = malloc(sizeof(struct chip8));
    if (c8 == NULL) {
        fprintf(stderr, "Could not malloc sctruct chip8");
        exit(1);
    }

    c8->mem = malloc(4096);
    c8->pc = 0;
    c8->deley_timer = 0;
    c8->sound_timer = 0;

    return c8;
}

void free_chip8(struct chip8 *c8) {
    free(c8->mem);
    free(c8);
}

void chip8_clr_bg(){
    ClearBackground(RAYWHITE);
}

void chip8_jump(struct chip8 *c8, uint16_t NNN){
    c8->pc = NNN;
}

void chip8_set_reg(struct chip8 *c8, uint8_t X, uint8_t NN){
    c8->V[X] = NN;
}

void chip8_add_reg(struct chip8 *c8, uint8_t X, uint8_t NN){
    c8->V[X] += NN;
}

void chip8_draw_sprite(struct chip8 *c8){
    assert(false && "TODO: needs to be implemented");
}


int main(void) {
    struct chip8 *c8 = init_chip8();

    // Start raylib
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "CHIP 8 Emulation");

    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();

        uint8_t nibble1, X, Y, N, NN;
        uint16_t NNN;
        switch (nibble1) {
        case 0x0:               /* Clear screen */
            chip8_clr_bg();
            break;
        case 0x1:               /* Jump */
            chip8_jump(c8, NNN);
            break;
        case 0x6:               /* Set register VX */
            chip8_set_reg(c8, X, NN);
            break;
        case 0x7:               /* Add value to register VX */
            chip8_add_reg(c8, X, NN);
            break;
        case 0xA:               /* Set index regiser I */
            break;
        case 0xD:               /* Draw a sprite */
            chip8_draw_sprite(c8);
            break;
        }

        // Draw
        {

            DrawText("Congrats! You created your first window!", 190, 200, 20,
                     LIGHTGRAY);
        }
        EndDrawing();
    }

    CloseWindow();
    free_chip8(c8);
    return 0;
}
