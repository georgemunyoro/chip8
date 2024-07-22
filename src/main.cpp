
#include "chip8.h"

#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Invalid number of args! Correct usage is:\n\tchip8 <ROM "
               "filepath>\n");
        exit(1);
    }

    Chip8 emulator(argv[1]);
    emulator.run();
}
