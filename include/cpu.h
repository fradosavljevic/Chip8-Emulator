#ifndef CPU_H
#define CPU_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    KEY_RELEASED = 0,
    KEY_PRESSED  = 1
} KeyState;

typedef struct {
    uint8_t  memory[4096]; //4KB
    uint8_t  V[16];          
    uint16_t I;              
    uint16_t pc;             
    uint16_t stack[16];      
    uint8_t  sp;             
    uint8_t  delay_timer;
    uint8_t  sound_timer;
    uint8_t  gfx[64 * 32];   
    KeyState  keypad[16];        
} Chip8;

typedef struct {
    uint16_t opcode;
    uint16_t nnn;
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint8_t kk;
    uint8_t op;
} DecodedOpcode;

bool chip8_loadROM(Chip8 *cpu, char *file);
void chip8_init(Chip8 *cpu);
void chip8_cycle(Chip8 *cpu);
void chip8_clsret(Chip8 *cpu, DecodedOpcode *decoded);
void chip8_exec8xy(Chip8 *cpu, DecodedOpcode *decoded);
void chip8_execEx(Chip8 *cpu, DecodedOpcode *decoded);
void chip8_execFx(Chip8 *cpu, DecodedOpcode *decoded);
uint16_t chip8_fetch(Chip8 *cpu);
DecodedOpcode chip8_decode(uint16_t opcode);

#endif