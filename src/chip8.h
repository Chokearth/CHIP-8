#pragma once

#include <iostream>
#include <cstring>
#include <cstdio>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHIP_8_MEMORY 4096
#define CHIP_8_REGISTER 16
#define CHIP_8_STACK 16
#define CHIP_8_SCREEN_WIDTH 64
#define CHIP_8_SCREEN_HEIGHT 32

class chip8 {
private:
    unsigned short opcode;

    unsigned char memory[CHIP_8_MEMORY];
    unsigned char V[CHIP_8_REGISTER];

    unsigned short I;
    unsigned short pc;

    unsigned char delay_timer;
    unsigned char sound_timer;

    unsigned short stack[CHIP_8_STACK];
    unsigned short sp;

public:

    unsigned char gfx[CHIP_8_SCREEN_WIDTH * CHIP_8_SCREEN_HEIGHT];

    unsigned char key[16];
    bool drawFlag = false;

    void initialize();
    void loadGame(const char *gamePath);
    void emulateCycle();
    void setKeys();

    void debug();
};
