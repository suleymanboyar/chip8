/*
 * This file is for referece only, does not get used by the core
 * program as of now.
 */

void print_opcode_info(opcode *op) {
    printf("Opcode: %x - ", op->bytes);
    switch (op->head) {
    case 0x0: /* Clear screen */
        switch (op->NN) {
        case 0xE0:
            printf("Clear Screen\n");
            break;
        case 0xEE:
            printf("Pop stack, set PC\n");
            break;
        }
        break;
    case 0x1: /* Jump */
        printf("Jump PC to NNN\n");
        break;
    case 0x2:
        printf("Push PC to stack, set PC to NNN\n");
        break;
    case 0x3:
        printf("Skip instruction, if VX == NN\n");
        break;
    case 0x4:
        printf("Skip instruction, if VX != NN\n");
        break;
    case 0x5:
        printf("Skip instruction, if VY == NN\n");
        break;
    case 0x6:
        printf("Set VX to NN\n");
        break;
    case 0x7:
        printf("Add NN to VX\n");
        break;
    case 0x8:
        switch (op->N) {
        case 0x0:
            printf("Set VX to VY\n");
            break;
        case 0x1:
            printf("OR VX with VY\n");
            break;
        case 0x2:
            printf("AND VX with VY\n");
            break;
        case 0x3:
            printf("XOR VX with VY\n");
            break;
        case 0x4:
            printf("Add VX + VY to VX\n");
            break;
        case 0x5:
            printf("Sub VX - VY to VX\n");
            break;
        case 0x6:
            printf("Shift VX one bit to right\n");
            break;
        case 0x7:
            printf("Sub VY - VX to VX\n");
            break;
        case 0xe:
            printf("Shift VX one bit to left\n");
            break;
        }
        break;
    case 0x9:
        printf("Skip instruction, if VY == NN\n");
        break;
    case 0xA: /* Set index regiser I */
        printf("Set memory in I to NNN\n");
        break;
    case 0xB:
        printf("Jump with offset, PC to NNN+VX\n");
        break;
    case 0xC: {
        printf("AND random number with NN, put in VX\n");
        break;
    } break;
    case 0xD: /* Draw a sprite */
        printf("Draw a sprit\n");
        break;
    case 0xE:
        switch (op->NN) {
        case 0x9E:
            printf("Jump to next instruction if key is pressed\n");
            break;
        case 0xA1:
            printf("Jump to next instruction if key is not pressed\n");
            break;
        }
        break;
    case 0xF:
        switch (op->NN) {
        // Timers
        case 0x7:
            printf("Set VX to current delay timer\n");
            break;
        case 0x15:
            printf("Set delay timer to VX\n");
            break;
        case 0x18:
            printf("Set sound timer to VX\n");
            break;
            // Get key input
        case 0xA:
            printf("Wait for key\n");
            break;
        // Add to index
        case 0x1E:
            printf("Add I with VX\n");
            break;
        // Font character
        case 0x29:
            printf("Points I to the correct font sprite\n");
            break;
        // Binary-coded decimal conversionPermalink
        case 0x33:
            printf("Binary-coded decimal\n");
            break;
        case 0x55:
            printf("Stores V0 -> VX in Mem[I->I+X]\n");
            break;
        case 0x65:
            printf("Stores Mem[I->I+X] in V0 -> VX\n");
            break;
        }
        break;
    }
}

int debug(uint8_t *ram, opcode *op) {
    print_opcode_info(op);
    char str[100];
    do {
        printf("[n]ext, [o]pcode, [r]ram, [q]uite: ");
        fgets(str, 100, stdin);
        switch (str[0]) {
        case 'n':
            break;
        case '\n':
            break;
        case 'o':
            print_opcode(*op);
            break;
        case 'r':
            print_bytes(ram, RAM_SIZE);
            break;
        case 'q':
            return 1;
        default:
            printf("Wrong input: %s\n", str);
        }

    } while (str[0] != 'n');
    return 0;
}


void print_stack(stack *sp) {
    if (sp == NULL) {
        printf("stack emptry\n");
    }

    printf("stack(%d): [%d]", sp->size, sp->data);

    stack *cur = sp;
    while (cur->next != NULL) {
        cur = cur->next;
        printf(" -> [%d]", cur->data);
    }
    printf("\n");
}

void print_texture(Texture2D texture) {
    Image image = LoadImageFromTexture(texture);
    Color *colors = LoadImageColors(image);
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            Color pixel = GetPixelColor(colors + x + y * texture.width, 0);
            if (pixel.r == BLACK.r && pixel.b == BLACK.g && pixel.g == BLACK.g)
                printf("[B] ");
            else
                printf("[W] ");
        }
        printf("\n");
    }
    UnloadImageColors(colors);
    UnloadImage(image);
}


void print_opcode(opcode op) {
    printf("Head: %x\n"
           "X: %x\n"
           "Y: %x\n"
           "N: %x\n"
           "NN: %x\n"
           "NNN: %x\n\n",
           op.head, op.X, op.Y, op.N, op.NN, op.NNN);
}
