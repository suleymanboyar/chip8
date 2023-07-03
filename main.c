#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RAM_SIZE 4096
#define MEM_OFFSET 0x200
#define STACK_SIZE 16
#define FONT_OFFSET 0x50

#define SCALE 10
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define DEBUG_CHIP8 0

typedef struct stack stack;
struct stack {
    uint16_t data;
    uint8_t size;
    stack *next;
};

struct chip8 {
    uint8_t *ram; /* 4KB RAM */
    uint16_t pc;  /* Program counter, keeps track of where in memory it  */
    uint16_t I; /* Locaiton in memory where the executation will be happening */
    stack *sp;  /* stack pointer to nested-list of subroutines */
    RenderTexture2D display;
    uint8_t deley_timer; /* Decrements 60 per sec */
    uint8_t sound_timer; /* Makes a beep sound if its bigger than 0 */
    uint8_t V[16];       /* general purpose variable from V0 to VF */
    uint8_t font_size;   /* Amount of bytes on font takes up */
    uint16_t rom_size;
};

uint8_t font[] = {0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
                  0x20, 0x60, 0x20, 0x20, 0x70,  // 1
                  0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
                  0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
                  0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
                  0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
                  0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
                  0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
                  0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
                  0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
                  0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
                  0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
                  0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
                  0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
                  0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
                  0xF0, 0x80, 0xF0, 0x80, 0x80}; // F

typedef struct {
    // DXYN, 0XNN, 1NNN
    uint8_t head;   /* First nibble in opcode */
    uint8_t X;      /* Second nibble in opcode */
    uint8_t Y;      /* Third nibble in opcode */
    uint8_t N;      /* Fourth nibble in opcode */
    uint8_t NN;     /* Third to forth byte in opcode */
    uint16_t NNN;   /* Second to forht 12 bit in opcode */
    uint16_t bytes; /* The whole opcode */
} opcode;

void print_bytes(const void *ptr, size_t size) {
    const unsigned char *p = ptr;
    for (size_t i = 0; i < size; i++) {
        printf("%02hhx", p[i]);
        if ((i + 1) % 2 == 0) {
            printf(" "); // Add extra space every 2 bytes for formatting
        }
        if ((i + 1) % 16 == 0) {
            printf("\n"); // Start a new line every 16 bytes for formatting
        }
    }
    printf("\n");
}

bool valid_file(char *file) {
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        perror("fopen");
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

int read_rom_file(char *file, uint8_t *dest, uint16_t *size) {
    FILE *f = fopen(file, "rb");
    if (!f) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Check if the file provided fits in side of the RAM

    // Read the file's bytes into the buffer
    size_t bytes_read = fread(dest + MEM_OFFSET, 1, *size, f);
    if (bytes_read != *size) {
        fprintf(stderr, "error: %zu != %d\n", bytes_read, *size);
        perror("fread");
        fclose(f);
        return EXIT_FAILURE;
    }

    fclose(f);
    return EXIT_SUCCESS;
}
stack *new_stack_entry(uint16_t value) {
    stack *sp = malloc(sizeof(stack));
    if (sp == NULL) {
        perror("stack malloc");
        exit(1);
    }
    sp->data = value;
    sp->size = 1;
    sp->next = NULL;
    return sp;
}

int ch8_stack_push(stack **sp, uint16_t value) {
    if (*sp == NULL) {
        *sp = new_stack_entry(value);
        return 0;
    }

    if ((*sp)->size == STACK_SIZE)
        return 1;

    (*sp)->size++;
    stack *cur = *sp;
    for (int i = 0; i < STACK_SIZE - 1; i++) {
        if (cur->next == NULL) {
            cur->next = new_stack_entry(value);
            break;
        }
        cur = cur->next;
    }

    return 0;
}

uint16_t ch8_stack_pop(stack **sp) {
    stack *tmp = *sp;
    uint16_t ret = (*sp)->data;

    if ((*sp)->next == NULL) {
        (*sp) = NULL;
    } else {
        *sp = (*sp)->next;
        (*sp)->size = --tmp->size;
    }

    free(tmp);
    return ret;
}

void free_chip8(struct chip8 *ch8) {
    free(ch8->ram);
    UnloadRenderTexture(ch8->display);
    free(ch8);
}

struct chip8 *init_chip8(char *rom_file) {
    struct chip8 *ch8 = malloc(sizeof(struct chip8));
    if (ch8 == NULL) {
        fprintf(stderr, "Could not malloc sctruct chip8");
        exit(1);
    }

    ch8->ram = calloc(RAM_SIZE, sizeof(ch8->ram));
    if (ch8->ram == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (read_rom_file(rom_file, ch8->ram, &ch8->rom_size) == EXIT_FAILURE) {
        free_chip8(ch8);
        exit(EXIT_FAILURE);
    }

    // insert fonts in ram
    memcpy(ch8->ram + FONT_OFFSET, &font, sizeof(font) / sizeof(font[0]));

    // Init display

    ch8->display = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    BeginTextureMode(ch8->display);
    ClearBackground(BLACK);
    EndTextureMode();

    ch8->pc = MEM_OFFSET;
    ch8->sp = NULL;
    ch8->deley_timer = 0;
    ch8->sound_timer = 0;
    ch8->font_size = 5;

    return ch8;
}

opcode get_opcode(uint8_t *mem, uint16_t pc) {
    opcode op;
    op.bytes = (((uint16_t)mem[pc] << 8) | mem[pc + 1]);
    op.head = mem[pc] >> 4;
    op.X = mem[pc] & 0xf;
    op.Y = mem[pc + 1] >> 4;
    op.N = mem[pc + 1] & 0xf;
    op.NN = mem[pc + 1];
    uint16_t mask = 0x0fff;
    op.NNN = op.bytes & mask;

    return op;
}

void draw_screen(struct chip8 *ch8) {
    BeginDrawing();
    // Needs to be done this way since openGL has its (0,0) at bottom
    // left of the monitor.
    DrawTexturePro(ch8->display.texture,
                   (Rectangle){0, 0, (float)ch8->display.texture.width,
                               -(float)ch8->display.texture.height},
                   (Rectangle){0, 0, (float)ch8->display.texture.width * SCALE,
                               (float)ch8->display.texture.height * SCALE},
                   (Vector2){0, 0}, 0.0f, WHITE);
    EndDrawing();
}

void chip8_clr_bg(struct chip8 *ch8) {
    BeginTextureMode(ch8->display);
    ClearBackground(BLACK);
    EndTextureMode();
}

// Adds two u8 number and sets *dest to be equal that, and returns 1 if there
// was an overflow.
uint8_t ch8_u8_add_reg(uint8_t *dest, uint8_t n1, uint8_t n2) {
    *dest = n1 + n2;
    return (*dest < n1 || *dest < n2);
}

// Adds two u16 number and sets *dest to be equal that, and returns 1 if there
// was an overflow.
uint8_t ch8_u16_add_reg(uint16_t *dest, uint16_t n1, uint16_t n2) {
    *dest = n1 + n2;
    return (*dest < n1 || *dest < n2);
}

// Subtacts two u8 number and sets *dest to be equal that, and returns 1 if there
// was an overflow, i.e becomes less than 0.
uint8_t ch8_u8_sub_reg(uint8_t *dest, uint8_t n1, uint8_t n2) {
    *dest = n1 - n2;
    return (n1 < n2);
}

// Shifts the *dest to 1 bit to the right and returns lsb.
uint8_t ch8_shr_reg(uint8_t *dest) {
    uint8_t lsb = *dest & 1;
    *dest >>= 1;
    return lsb;
}

// Shifts the *dest to 1 bit to the left and returns msb.
uint8_t ch8_shl_reg(uint8_t *dest) {
    uint8_t msb = *dest >> 7 & 1;
    *dest <<= 1;
    return msb;
}

// DXYN: Draws a sprite (8,op.N) pixels big by flipping the pixels on
// the screen if the bit in the sprite is HIGH and set VF=1.
void chip8_draw_sprite(struct chip8 *ch8, opcode op) {
    Image img = LoadImageFromTexture(ch8->display.texture);
    Color *display = LoadImageColors(img);

    Vector2 pos = {.x = ch8->V[op.X] % SCREEN_WIDTH,
                   .y = ch8->V[op.Y] % SCREEN_HEIGHT};

    ch8->V[0xF] = 0;
    for (int y = 0; y < op.N; y++) {
        if (pos.y == SCREEN_HEIGHT)
            break;

        uint8_t sprite_byte = ch8->ram[ch8->I + y];

        for (uint8_t x = 0; x <8; x++) {
            if (pos.x+x == SCREEN_WIDTH)
                break;

            uint8_t sprite_pixel = sprite_byte & (0x80 >> x);

            if (sprite_pixel) {
                uint8_t index = (pos.x + x) + (pos.y+y) * SCREEN_WIDTH;
                uint32_t hex_color = ColorToInt(GetPixelColor(
                    &display[index], PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));

                Color color = WHITE;
                if (hex_color == 0xFFFFFFFF){
                    ch8->V[0xF] = 1;
                    color = BLACK;
                } 

                BeginTextureMode(ch8->display);
                DrawRectangle(pos.x+x, pos.y+y, 1, 1, color);
                EndTextureMode();
            }
        }
    }
    UnloadImageColors(display);
    UnloadImage(img);
}


// Checks if the given hex value key is being pressed.
bool chip8_is_key_pressed(uint8_t key) {
    uint16_t keys[16] = {KEY_ZERO,  KEY_ONE,  KEY_TWO, KEY_THREE,
                         KEY_FOUR,  KEY_FIVE, KEY_SIX, KEY_SEVEN,
                         KEY_EIGHT, KEY_NINE, KEY_A,   KEY_B,
                         KEY_C,     KEY_D,    KEY_E,   KEY_F};

    return IsKeyDown(keys[key]);
}

// Checks if the given any keys is being pressed, if so return its
// posision.
uint8_t chip8_which_key_pressed(void) {
    uint8_t keys[16] = {KEY_ZERO,  KEY_ONE,  KEY_TWO, KEY_THREE,
                        KEY_FOUR,  KEY_FIVE, KEY_SIX, KEY_SEVEN,
                        KEY_EIGHT, KEY_NINE, KEY_A,   KEY_B,
                        KEY_C,     KEY_D,    KEY_E,   KEY_F};

    for (int i = 0; i < 16; i++) {
        if (IsKeyDown(keys[i]))
            return i;
    }

    return KEY_NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arugnemtns: ./main <rom.ch8>\n");
        return 1;
    }
    char *rom_file = argv[1];

    if (!valid_file(rom_file)) {
        fprintf(stderr, "Wrong file was given: %s\n", rom_file);
        return 1;
    }

    // XXX: Maybe set a random seed;
    srand(0);

    // Start raylib
    // SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, "CHIP 8 Emulation");
    SetTargetFPS(60);

    struct chip8 *ch8 = init_chip8(rom_file);
    printf("loading file: %s\n", rom_file);
    print_bytes(ch8->ram + MEM_OFFSET, ch8->rom_size);

    while (!WindowShouldClose()) {
        opcode op = get_opcode(ch8->ram, ch8->pc);

        ch8->pc += 2;

        switch (op.head) {
        case 0x0:
            switch (op.NN) {
            // Clear the display
            case 0xE0:
                chip8_clr_bg(ch8);
                break;
            // Set PC to the first subroutine in the stack
            case 0xEE:
                ch8->pc = ch8_stack_pop(&ch8->sp);
                break;
            }
            break;
        // Make PC jum to address NNN
        case 0x1:
            ch8->pc = op.NNN;
            break;
        // Push the current PC address to stack and set it to NNN
        case 0x2:
            ch8_stack_push(&ch8->sp, ch8->pc);
            ch8->pc = op.NNN;
            break;
         // Jump to next opcode if VX == NN
        case 0x3:
            if (ch8->V[op.X] == op.NN)
                ch8->pc += 2;
            break;
         // Jump to next opcode if VX != NN
        case 0x4:
            if (ch8->V[op.X] != op.NN)
                ch8->pc += 2;
            break;
         // Jump to next opcode if VX == VY
        case 0x5:
            if (ch8->V[op.X] == ch8->V[op.Y])
                ch8->pc += 2;
            break;
         // Set VX = NN
        case 0x6: // Set register VX
            ch8->V[op.X] = op.NN;
            break;
         // Add VX += NN
        case 0x7: // Add value to register VX
            ch8->V[op.X] += op.NN;
            break;
        case 0x8:
            switch (op.N) {
            // Set VX = VF
            case 0x0:
                ch8->V[op.X] = ch8->V[op.Y];
                break;
            // Set VX = VX OR VY
            case 0x1:
                ch8->V[op.X] |= ch8->V[op.Y];
                break;
            // Set VX = VX AND VY
            case 0x2:
                ch8->V[op.X] &= ch8->V[op.Y];
                break;
            // Set VX = VX XOR VY
            case 0x3:
                ch8->V[op.X] ^= ch8->V[op.Y];
                break;
            // Set VX += VY, and VF = 1 if overflow else 0
            case 0x4:
                ch8->V[0xF] =
                    ch8_u8_add_reg(&ch8->V[op.X], ch8->V[op.X], ch8->V[op.Y]);
                break;
            // Set VX -= VY, and VF = 1 if overflow else 0
            case 0x5:
                ch8->V[0xF] =
                    !ch8_u8_sub_reg(&ch8->V[op.X], ch8->V[op.X], ch8->V[op.Y]);
                break;
            // Set VX >> 1, VF=LSB
            case 0x6:
                ch8->V[0xF] = ch8_shr_reg(&ch8->V[op.X]);
                break;
             // Set VX << 1, VF=MSB
            case 0x7:
                ch8->V[0xF] =
                    !ch8_u8_sub_reg(&ch8->V[op.X], ch8->V[op.Y], ch8->V[op.X]);
                break;
            // Set VX = VY - VX
            case 0xe:
                ch8->V[0xF] = ch8_shl_reg(&ch8->V[op.X]);
                break;
            }
            break;
        // 9XY0 - Skip if VX != VY
        case 0x9:
            if (ch8->V[op.X] != ch8->V[op.Y])
                ch8->pc += 2;
            break;
         // ANNN - I = NNN
        case 0xA:
            ch8->I = op.NNN;
            break;
        // BNNN - Jump to V0 + NNN
        // COSMAC uses just NNN but later versions of chip8 started to
        // use NNN + VX
        case 0xB:
#ifdef COSMAC
            ch8->pc = op.NNN;
#else
            ch8->pc = op.NNN + ch8->V[op.X];
#endif
            break;
        // CXNN - VX = rand() & NN
        case 0xC:
            ch8->V[op.X] = ((uint8_t)rand()) % 256 & op.NN;
            break;
         // DXYN - Draw Sprite
        case 0xD:
            chip8_draw_sprite(ch8, op);
            break;
        case 0xE:
            switch (op.NN) {
            // EX9E - Skip if Key Pressed
            case 0x9E:
                if (chip8_is_key_pressed(ch8->V[op.X]))
                    ch8->pc += 2;
                break;
            // EXA1 - Skip if Key Not Pressed
            case 0xA1:
                if (!chip8_is_key_pressed(ch8->V[op.X]))
                    ch8->pc += 2;
                break;
            }
            break;
        case 0xF:
            switch (op.NN) {
            // FX07 - VX = DT, sets VX to  deley timer
            case 0x7:
                ch8->V[op.X] = ch8->deley_timer;
                break;
            // FX0A - Wait for Key Press
            case 0xA: {
                uint8_t key = chip8_which_key_pressed();
                if (key != KEY_NULL)
                    ch8->V[op.X] = key;
                else
                    ch8->pc -= 2;
            } break;
            // FX15 - DT = VX, sets delay timer to DT
            case 0x15:
                ch8->deley_timer = ch8->V[op.X];
                break;
            // FX18 - ST = VX, sets sound timer to VX
            case 0x18:
                ch8->sound_timer = ch8->V[op.X];
                break;
            // FX1E - I += VX
            case 0x1E:
                ch8->V[0xF] =
                    ch8_u16_add_reg(&ch8->I, ch8->I, (uint16_t)ch8->V[op.X]);
                break;
            // FX29 - Set I to Font Address
            case 0x29:
                ch8->I = FONT_OFFSET + ch8->font_size * (ch8->V[op.X] & 0xF);
                break;
            // FX33 - I = BCD of VX
            case 0x33:
                ch8->ram[ch8->I] = ch8->V[op.X] / 100;
                ch8->ram[ch8->I + 1] = ch8->V[op.X] % 100 / 10;
                ch8->ram[ch8->I + 2] = ch8->V[op.X] % 10;
                break;
            // FX55 - Store V0 - VX into I
            case 0x55:
                for (int i = 0; i <= op.X; i++)
                    ch8->ram[ch8->I + i] = ch8->V[i];
                break;
            // FX65 - Load I into V0 - VX
            case 0x65:
                for (int i = 0; i <= op.X; i++)
                    ch8->V[i] = ch8->ram[ch8->I + i];
                break;
            }
            break;
        }

        // Draw texture
        draw_screen(ch8);

        // TODO: make a beep sound if ch8->sourd_timer is greater than 0
    }

    free_chip8(ch8);
    return 0;
}
