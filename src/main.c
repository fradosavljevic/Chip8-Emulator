#include <stdio.h>
#include "../include/cpu.h"

// Temporary function for testing
void chip8_render_console(Chip8 *cpu) {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            printf("%c", cpu->gfx[y * 64 + x] ? '#' : '.');
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <path_to_rom>\n", argv[0]);
        return 1;
    }
    
    srand(time(NULL));
    
    Chip8 cpu;
    chip8_init(&cpu);
    bool status = chip8_loadROM(&cpu, argv[1]);
    if(!status) {
        perror("Something went wrong! Your rom wasn't loaded...");
        return 0;
    }

    printf("CHIP-8 emulator initialised!\n");
    printf("PC address: 0x%X\n", cpu.pc);

    int cycles = 0;
    // Increase if needed
    while (cycles < 400) { 
        chip8_cycle(&cpu);
        cycles++;
    }

    chip8_render_console(&cpu);
    return 0;
}