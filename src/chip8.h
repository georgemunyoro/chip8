#pragma once

#include <SDL_render.h>
#include <SDL_video.h>
#include <_types/_uint16_t.h>
#include <_types/_uint64_t.h>
#include <_types/_uint8_t.h>

struct Chip8SDLDisplay {
  uint64_t bits[32];
  SDL_Window *window;
  SDL_Surface *screenSurface;
  SDL_Renderer *renderer;
  int width, height;
};

class Chip8 {
public:
  Chip8(char *romFilePath);

  void run();

private:
  void clearMemory();
  void loadROMFileFromPath(char *romFilePath);
  void drawDisplayToTerminal();

  /* Memory Map:
    +---------------+= 0xFFF (4095) End of Chip-8 RAM
    |               |
    |               |
    |               |
    |               |
    |               |
    | 0x200 to 0xFFF|
    |     Chip-8    |
    | Program / Data|
    |     Space     |
    |               |
    |               |
    |               |
    +- - - - - - - -+= 0x600 (1536) Start of ETI 660 Chip-8 programs
    |               |
    |               |
    |               |
    +---------------+= 0x200 (512) Start of most Chip-8 programs
    | 0x000 to 0x1FF|
    | Reserved for  |
    |  interpreter  |
    +---------------+= 0x000 (0) Start of Chip-8 RAM */
  uint8_t memory[4096];

  // Chip-8 has 16 general purpose 8-bit registers, usually referred to as Vx,
  // where x is a hexadecimal digit (0 through F).
  uint8_t V[16];

  // There is also a 16-bit register called I. This register is generally used
  // to store memory addresses, so only the lowest (rightmost) 12 bits are
  // usually used.
  uint16_t I;

  /*
    Chip-8 provides 2 timers, a delay timer and a sound timer.
  */

  // The delay timer is active whenever the delay timer register (DT) is
  // non-zero. This timer does nothing more than subtract 1 from the value of DT
  // at a rate of 60Hz. When DT reaches 0, it deactivates.
  uint8_t delayTimer;

  // The sound timer is active whenever the sound timer register (ST) is
  // non-zero. This timer also decrements at a rate of 60Hz, however, as long as
  // ST's value is greater than zero, the Chip-8 buzzer will sound. When ST
  // reaches zero, the sound timer deactivates.
  //
  // The sound produced by the Chip-8 interpreter has only one tone. The
  // frequency of this tone is decided by the author of the interpreter.
  uint8_t soundTimer;

  // The program counter (PC) should be 16-bit, and is used to store the
  // currently executing address.
  uint16_t PC;

  // The stack pointer (SP) can be 8-bit, it is used to point to the topmost
  // level of the stack.
  uint8_t SP;

  // The stack is an array of 16 16-bit values, used to store the address that
  // the interpreter shoud return to when finished with a subroutine. Chip-8
  // allows for up to 16 levels of nested subroutines.
  uint16_t stack[16];

  /*
    The computers which originally used the Chip-8 Language had a 16-key
    hexadecimal keypad with the following layout:

      1	2 3 C
      4	5 6 D
      7	8 9 E
      A	0 B F

    In this emulator, it's mapped to modern keyboards as such:

      1 2 3 4
      q w e r
      a s d f
      z x c v
  */
  uint16_t keyboard;

  /*
    The original implementation of the Chip-8 language used a 64x32-pixel
    monochrome display with this format:

        ----------------------
        |(0, 0)       (63, 0)|
        |                    |
        |                    |
        |(0, 31)     (63, 31)|
        ----------------------
  */
  Chip8SDLDisplay display;
};
