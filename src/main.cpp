#include <SDL2/SDL.h>
#include <SDL_pixels.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include <cairo.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "chip8.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Invalid number of args! Correct usage is:\n\tchip8 <ROM "
               "filepath>\n");
        exit(1);
    }

    Chip8* c = new Chip8(argv[1]);
    c->run();
}
