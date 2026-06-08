#include "../include/cpu.h"
#include "../include/font.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void chip8_init(Chip8 *cpu) {
    memset(cpu, 0, sizeof(Chip8));
    for(int loc = 0x0; loc < 0x80; loc++)
        cpu->memory[loc] = chip8_fontset[loc];
    cpu->pc = 0x200;
}

bool chip8_loadROM(Chip8 *cpu, char *file) {
    FILE *f = NULL;
    errno_t err = fopen_s(&f, file, "rb");
    if(err != 0) return false;
    
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    rewind(f);

    if (fileSize > (4096 - 0x200)) {
        printf("Error: The file is too big (%ld bytes).\n", fileSize);
        fclose(f);
        return false;
    }

    fread(cpu->memory + 0x200, 1, fileSize, f);
    fclose(f);
    return true;
}

uint16_t chip8_fetch(Chip8 *cpu) {
    uint16_t high = (cpu->memory[cpu->pc] << 8);
    uint16_t low = (cpu->memory[cpu->pc + 1]);
    uint16_t opcode = high | low;
    cpu->pc += 2;
    return opcode;
}

DecodedOpcode chip8_decode(uint16_t opcode) {
    DecodedOpcode decoded;
    decoded.opcode = opcode;
    decoded.n = opcode & 0x000F;
    decoded.kk = opcode & 0x00FF;
    decoded.nnn = opcode & 0x0FFF;
    decoded.x = (opcode & 0x0F00) >> 8;
    decoded.y = (opcode & 0x00F0) >> 4;
    decoded.op = (opcode & 0xF000) >> 12;
    return decoded;
}

void chip8_clsret(Chip8 *cpu, DecodedOpcode *decoded) {
    if(decoded->opcode == 0x00E0) {
        memset(cpu->gfx, 0, sizeof(cpu->gfx)); 
    } 
    else if (decoded->opcode == 0x00EE) {
        cpu->pc = cpu->stack[cpu->sp--];
    }
}

void chip8_exec8xy(Chip8 *cpu, DecodedOpcode *decoded) {
    switch(decoded->n) {
        case 0x0:
            cpu->V[decoded->x] = cpu->V[decoded->y];
            break;
        case 0x1:
            cpu->V[decoded->x] |= cpu->V[decoded->y];
            break;
        case 0x2:
            cpu->V[decoded->x] &= cpu->V[decoded->y];
            break;
        case 0x3:
            cpu->V[decoded->x] ^= cpu->V[decoded->y];
            break;
        case 0x4:
            uint16_t sum = cpu->V[decoded->x] + cpu->V[decoded->y];
            cpu->V[decoded->x] = sum & 0x00FF;
            cpu->V[0xF] = ((sum >> 8) > 0) ? 1 : 0; 
            break;
        case 0x5: 
            cpu->V[0xF] = (cpu->V[decoded->x] >= cpu->V[decoded->y] ? 1 : 0);
            cpu->V[decoded->x] -= cpu->V[decoded->y];
            break;
        case 0x6: //modern chip8
            cpu->V[0xF] = cpu->V[decoded->x] & 1;
            cpu->V[decoded->x] >>= 1;
            break;
        case 0x7: 
            cpu->V[0xF] = (cpu->V[decoded->y] >= cpu->V[decoded->x] ? 1 : 0);
            cpu->V[decoded->x] = cpu->V[decoded->y] - cpu->V[decoded->x];
            break;
        case 0xE: //modern chip8
            cpu->V[0xF] = (cpu->V[decoded->x] >> 7) & 1;
            cpu->V[decoded->x] <<= 1;
            break;
    }
}

void chip8_execEx(Chip8 *cpu, DecodedOpcode *decoded) {
    uint8_t key = cpu->V[decoded->x] & 0x0F; // prevent potential overflow
    if(decoded->kk == 0x9E) {
        cpu->pc += (cpu->keypad[key] == KEY_PRESSED ? 2 : 0);
    } else {
        cpu->pc += (cpu->keypad[key] == KEY_RELEASED ? 2 : 0);
    }
}

void chip8_execFx(Chip8 *cpu, DecodedOpcode *decoded) {
    switch (decoded->kk) {
        case 0x07: 
            cpu->V[decoded->x] = cpu->delay_timer;
            break;
        case 0x0A: 
            int key_pressed = 0;
            for (int i = 0; i < 16; i++) {
                if (cpu->keypad[i] == KEY_PRESSED) {
                    cpu->V[decoded->x] = i;
                    key_pressed = 1;
                    break;
                }
            }

            if (!key_pressed) cpu->pc -= 2; 
            break; 
        case 0x15: 
            cpu->delay_timer = cpu->V[decoded->x];
            break;
        case 0x18:
            cpu->sound_timer = cpu->V[decoded->x];
            break;
        case 0x1E: 
            cpu->I += cpu->V[decoded->x];
            break;
        case 0x29: 
            cpu->I = (cpu->V[decoded->x] & 0x0F) * 5;
            break;
        case 0x33: 
            cpu->memory[cpu->I]     = (cpu->V[decoded->x] / 100) % 10;
            cpu->memory[cpu->I + 1] = (cpu->V[decoded->x] / 10) % 10;
            cpu->memory[cpu->I + 2] = (cpu->V[decoded->x] % 10);
            break;
        case 0x55: 
            for(int loc = 0; loc <= decoded->x; loc++)
                cpu->memory[cpu->I + loc] = cpu->V[loc];
            cpu->I += (decoded->x + 1);
            break;
        case 0x65: 
            for(int loc = 0; loc <= decoded->x; loc++)
                cpu->V[loc] = cpu->memory[cpu->I + loc];
            cpu->I += (decoded->x + 1);
            break;
    }
}

void chip8_cycle(Chip8 *cpu) {
    uint16_t opcode = chip8_fetch(cpu);  
    DecodedOpcode decoded = chip8_decode(opcode);  
    switch (decoded.op) {
        case 0x0:
            chip8_clsret(cpu, &decoded); 
            break;
        case 0x1:
            cpu->pc = decoded.nnn; 
            break;
        case 0x2:
            if (cpu->sp < 15) {
                cpu->stack[++cpu->sp] = cpu->pc;
                cpu->pc = decoded.nnn;
            }
            break;
        case 0x3: 
            if(cpu->V[decoded.x] == decoded.kk) cpu->pc += 2;
            break;
        case 0x4: 
            if(cpu->V[decoded.x] != decoded.kk) cpu->pc += 2;
            break;
        case 0x5: 
            if(cpu->V[decoded.x] == cpu->V[decoded.y]) cpu->pc += 2;
            break;
        case 0x6: 
            cpu->V[decoded.x] = decoded.kk;    
            break;
        case 0x7: 
            cpu->V[decoded.x] += decoded.kk;
            break;
        case 0x8: 
            chip8_exec8xy(cpu, &decoded);
            break;
        case 0x9: 
            if(cpu->V[decoded.x] != cpu->V[decoded.y]) cpu->pc += 2;
            break;
        case 0xA: 
            cpu->I = decoded.nnn;
            break;
        case 0xB: //modern chip8
            cpu->pc = decoded.nnn + cpu->V[0];
            break;
        case 0xC:
            cpu->V[decoded.x] = (rand() % 256) & decoded.kk;
            break;
        case 0xD:
            cpu->V[0xF] = 0;
            for(int row = 0; row < decoded.n; row++) {
                uint8_t sprite = cpu->memory[cpu->I + row];
                for(int col = 0; col < 8; col++) {
                    if(sprite & (0x80 >> col)) {
                        int screen_x = (cpu->V[decoded.x] + col) % 64;
                        int screen_y = (cpu->V[decoded.y] + row) % 32;
                        cpu->V[0xF] = (cpu->gfx[screen_y * 64 + screen_x] == 1 ? 1 : cpu->V[0xF]);
                        cpu->gfx[screen_y * 64 + screen_x] ^= 1;
                    }
                }
            }
            break;
        case 0xE: 
            chip8_execEx(cpu, &decoded);
            break;
        case 0xF: 
            chip8_execFx(cpu, &decoded);
            break;
    }
}