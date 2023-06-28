#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MEM_SIZE 4096
#define MEM_OFFSET 0x200

#define SCALE 10
#define SCREEN_WIDTH 64 * SCALE
#define SCREEN_HEIGHT 32 * SCALE

struct chip8 {
    uint8_t *ram; /* 4KB RAM */
    uint16_t pc;  /* Program counter, keeps track of where in memory it  */
    uint16_t *I; /* Locaiton in memory where the executation will be happening */
    uint8_t deley_timer; /* Decrements 60 per sec */
    uint8_t sound_timer; /* Makes a beep sound if its bigger than 0 */
    uint8_t V[16];       /* general purpose variable from V0 to VF */
    uint16_t rom_size;
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

void pirnt_opcode(opcode op) {
    printf("Opcode: %x\n"
           "Head: %x\n"
           "X: %x\n"
           "Y: %x\n"
           "N: %x\n"
           "NN: %x\n"
           "NNN: %x\n",
           op.bytes, op.head, op.X, op.Y, op.N, op.NN, op.NNN);
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

void free_chip8(struct chip8 *ch8) {
    free(ch8->ram);
    free(ch8);
}

struct chip8 *init_chip8(char *rom_file) {
    struct chip8 *ch8 = malloc(sizeof(struct chip8));
    if (ch8 == NULL) {
        fprintf(stderr, "Could not malloc sctruct chip8");
        exit(1);
    }

    ch8->ram = malloc(MEM_SIZE);
    if (ch8->ram == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (read_rom_file(rom_file, ch8->ram, &ch8->rom_size) == EXIT_FAILURE) {
        free_chip8(ch8);
        exit(EXIT_FAILURE);
    }

    ch8->pc = MEM_OFFSET;
    ch8->deley_timer = 0;
    ch8->sound_timer = 0;

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

uint8_t check_u8_underflow(uint8_t num) { return (uint16_t)num > 255; }

void chip8_clr_bg() { ClearBackground(RAYWHITE); }

void chip8_jump(struct chip8 *ch8, uint16_t NNN) {
    ch8->pc = NNN;
    ch8->pc -= 2;
}

/* Set the register dest to the value src */
void chip8_set_reg(uint8_t *dest, uint8_t src) { *dest = src; }

/* Add the value NN to VX */
void chip8_add_reg(uint8_t *dest, uint8_t src) { *dest += src; }

void chip8_or_reg(uint8_t *dest, uint8_t src) { *dest |= src; }

void chip8_and_reg(uint8_t *dest, uint8_t src) { *dest &= src; }

void chip8_xor_reg(uint8_t *dest, uint8_t src) { *dest ^= src; }

void chip8_sub_reg(uint8_t *dest, uint8_t src) { *dest = src; }

void chip8_shr_reg(uint8_t *dest, uint8_t src) { *dest >>= src; }

void chip8_shl_reg(uint8_t *dest, uint8_t src) { *dest <<= src; }

void chip8_draw_sprite(struct chip8 *ch8) {
    assert(false && "TODO: needs to be implemented");
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

    struct chip8 *ch8 = init_chip8(rom_file);

    printf("loading file: %s\n", rom_file);
    print_bytes(ch8->ram + MEM_OFFSET, ch8->rom_size);

    // Start raylib
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "CHIP 8 Emulation");

    printf("\n After raylib:\n");
    print_bytes(ch8->ram + MEM_OFFSET, ch8->rom_size);

    while (!WindowShouldClose()) {
        opcode op = get_opcode(ch8->ram, ch8->pc);
        pirnt_opcode(op);

        switch (op.head) {
        case 0x0: /* Clear screen */
            chip8_clr_bg();
            break;
        case 0x1:                    /* Jump */
            chip8_jump(ch8, op.NNN); /* Works */
            break;
        case 0x6: /* Set register VX */
            chip8_set_reg(&op.X, op.NN);
            break;
        case 0x7: /* Add value to register VX */
            chip8_add_reg(&op.X, op.NN);
            break;
        case 0x8:
            switch (op.N) {
            case 0x0:
                chip8_set_reg(&ch8->V[op.X], ch8->V[op.Y]);
                break;
            case 0x1:
                chip8_or_reg(&ch8->V[op.X], ch8->V[op.Y]);
                break;
            case 0x2:
                chip8_and_reg(&ch8->V[op.X], ch8->V[op.Y]);
                break;
            case 0x3:
                chip8_xor_reg(&ch8->V[op.X], ch8->V[op.Y]);
                break;
            case 0x4:
                chip8_add_reg(&ch8->V[op.X], ch8->V[op.X] + ch8->V[op.Y]);
                ch8->V[0xF] = check_u8_underflow(ch8->V[op.X] + ch8->V[op.Y]);
                break;
            case 0x5:
                chip8_sub_reg(&ch8->V[op.X], ch8->V[op.X] - ch8->V[op.Y]);
                ch8->V[0xF] = check_u8_underflow(ch8->V[op.X] - ch8->V[op.Y]);
                break;
            case 0x7:
                chip8_sub_reg(&ch8->V[op.X], ch8->V[op.Y] - ch8->V[op.X]);
                ch8->V[0xF] = check_u8_underflow(ch8->V[op.Y] - ch8->V[op.X]);
                break;
            case 0x6:
                ch8->V[0xF] = ch8->V[op.X] & 0xF;
                chip8_shr_reg(&ch8->V[op.X], ch8->V[op.X]);
                break;
            case 0xe:
                ch8->V[0xF] = ch8->V[op.X] & 0xF0;
                chip8_shl_reg(&ch8->V[op.X], ch8->V[op.X]);
                break;
            }
        case 0xA: /* Set index regiser I */
            break;
        case 0xD: /* Draw a sprite */
            chip8_draw_sprite(ch8);
            break;
        }
        ch8->pc += 2;
    }

    free_chip8(ch8);
    return 0;
}
