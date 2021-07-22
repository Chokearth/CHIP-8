//
// Created by chokearth on 09/07/2021.
//

#include "chip8.h"

unsigned char chip8_fontset[80] =
        {
                0xF0, 0x90, 0x90, 0x90, 0xF0, //0
                0x20, 0x60, 0x20, 0x20, 0x70, //1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
                0x90, 0x90, 0xF0, 0x10, 0x10, //4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
                0xF0, 0x10, 0x20, 0x40, 0x40, //7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
                0xF0, 0x90, 0xF0, 0x90, 0x90, //A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
                0xF0, 0x80, 0x80, 0x80, 0xF0, //C
                0xE0, 0x90, 0x90, 0x90, 0xE0, //D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
                0xF0, 0x80, 0xF0, 0x80, 0x80  //F
        };

void chip8::initialize() {
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    delay_timer = 0;
    sound_timer = 0;

    // Clear memory
    memset(memory, 0, CHIP_8_MEMORY);
    // Clear registers
    memset(V, 0, CHIP_8_REGISTER);
    // Clear display
    memset(gfx, 0, CHIP_8_SCREEN_WIDTH * CHIP_8_SCREEN_HEIGHT);
    // Clear stack
    memset(stack, 0, CHIP_8_STACK);

    // Load font
    memcpy(memory, chip8_fontset, 80);

    drawFlag = true;

    srand(time(NULL));
}

void chip8::loadGame(const char *gamePath) {
    FILE *gameFile = fopen(gamePath, "rb");

    if (gameFile) {

        fseek(gameFile, 0, SEEK_END);
        long lSize = ftell(gameFile);
        rewind(gameFile);

        char *buffer = (char *) malloc(sizeof(char) * lSize);
        fread(buffer, 1, lSize, gameFile);

        if ((CHIP_8_MEMORY - 512) > lSize) {
            for (int i = 0; i < lSize; i++) {
                memory[i + 512] = buffer[i];
            }
        }

    }
}

void chip8::emulateCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];

    switch (opcode & 0xF000) {
        case 0x0000:

            switch (opcode) {
                case 0x00E0: // 0x00E0 -> Clears the screen
                    memset(gfx, 0, CHIP_8_SCREEN_WIDTH * CHIP_8_SCREEN_HEIGHT);
                    drawFlag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 0x00EE -> Returns from subroutine
                    pc = stack[--sp];
                    pc += 2;
                    break;
            }
            break;

        case 0x1000: // 0x1NNN -> Jumps to address NNN
            pc = opcode & 0x0FFF;
            break;
        case 0x2000: // 0x2NNN -> Calls subroutine at NNN
            stack[sp++] = pc;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000: // 0x3XNN -> Skips next if V[X] == NN
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 2;
            pc += 2;
            break;
        case 0x4000: // 0x4XNN -> Skips next if V[X] != NN
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 2;
            pc += 2;
            break;
        case 0x5000: // 0x5XY0 -> Skips next if V[X] == V[Y]
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 2;
            pc += 2;
            break;
        case 0x6000: // 0x6XNN -> Sets V[X] to NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000: // 0x7XNN -> Adds NN to V[X]
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000:

            switch (opcode & 0x000F) {
                case 0x0000: // 0x8XY0 -> Sets V[X] to value of V[Y]
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0001: // 0x8XY1 -> Sets V[X] to V[X] or V[Y]
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0002: // 0x8XY2 -> Sets V[X] to V[X] and V[Y]
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0003: // 0x8XY3 -> Sets V[X] to V[X] xor V[Y]
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0004: // 0x8XY4 -> Adds V[Y] to V[X] (Flag to 1 when carry)
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1; // Carry
                    else
                        V[0xF] = 0;

                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0005: // 0x8XY5 -> Removes V[Y] to V[X] (Flag to 0 when borrow)
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                        V[0xF] = 0; // Borrow
                    else
                        V[0xF] = 1;

                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0006: // 0x8XY6 -> Stores the least significant bit of V[X] in VF and then shifts V[X] to the right by 1
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                    break;
                case 0x0007: // 0x8XY7 -> Sets V[X] to V[Y] minus V[X]. (Flag to 0 when borrow)
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                        V[0xF] = 0; // Borrow
                    else
                        V[0xF] = 1;

                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x000E: // 0x8XYE -> Stores the most significant bit of V[X] in VF and then shifts V[X] to the left by 1
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x80;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                    break;
            }
            break;

        case 0x9000: // 0x9XY0 -> Skips next if V[X] != V[Y]
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 2;
            pc += 2;
            break;
        case 0xA000: // 0xANNN -> Sets I to NNN
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xB000: // 0xBNNN -> Jumps to NNN+V[0]
            pc = (opcode & 0x0FFF) + V[0];
            pc += 2;
            break;
        case 0xC000: // CXNN -> Sets V[X] to the result of a bitwise and operation on a random number
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0xFF);
            pc += 2;
            break;
        case 0xD000: {// DXYN -> Draw the sprite at memory location I at coordinate (V[X], V[Y]) with a height of N+1 pixels (Flag to 1 if collision)
            unsigned char VX = V[(opcode & 0x0F00) >> 8];
            unsigned char VY = V[(opcode & 0x00F0) >> 4];
            unsigned char N = opcode & 0xF;
            unsigned short pixel;

            V[0xF] = 0;
            for (int y = 0; y < N; y++) {
                pixel = memory[I + y];
                for (int x = 0; x < 8; x++) {

                    if ((pixel & (0x80 >> x)) != 0) {
                        if (gfx[(VX + x + ((VY + y) * 64))] == 1)
                            V[0xF] = 1;
                        gfx[(VX + x + ((VY + y) * 64))] ^= 1;
                    }

                }
            }

            drawFlag = true;
            pc += 2;
            break;
        }
        case 0xE000:

            switch (opcode & 0x00FF) {
                case 0x009E: // EX9E -> Skips if key at V[X] is pressed
                    if ((key[V[(opcode & 0x0F00) >> 8]] & 0x1) != 0)
                        pc += 2;
                    pc += 2;
                    break;
                case 0x00A1: // EX9E -> Skips if key at V[X] is pressed
                    if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc += 2;
                    pc += 2;
                    break;
            }
            break;

        case 0xF000:

            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07 -> Sets V[X] to the value of the delay timer
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A: {// 0xFX0A -> Waits for a key press and store it in V[X]

                    bool keyPressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (key[i] != 0) {
                            keyPressed = true;
                            V[(opcode & 0x0F00) >> 8] = i;
                        }
                    }
                    if (!keyPressed) return;

                    pc += 2;
                    break;
                }
                case 0x0015: // 0xFX15 -> Sets the delay timer to V[X]
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0018: // 0xFX18 -> Sets the sound timer to V[X]
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x001E: // 0xFX1E -> Adds V[X] to I
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0029: // 0xFX19 -> Adds V[X] to I
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                    break;
                case 0x0033: // 0xFX33 -> store the digit of the digital representation of V[X] to I, I+1 and I+2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = V[(opcode & 0x0F00) >> 8] % 10;

                    pc += 2;
                    break;
                case 0x0055: // 0xFX55 -> Stores V0 to VX to memory starting from I
                    for (int i = 0; i < (opcode & 0x0F00) >> 8; i++)
                        memory[I + i] = V[i];

                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;
                case 0x0065: // 0xFX65 -> Fills V0 to VX from memory starting from I
                    for (int i = 0; i < (opcode & 0x0F00) >> 8; i++)
                        V[i] = memory[I + i];

                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;
            }
            break;
    }

    // Update timers
    if (delay_timer > 0)
        delay_timer--;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            std::cout << "BEEP!" << std::endl;
        sound_timer--;
    }
}