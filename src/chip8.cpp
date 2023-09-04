#include "chip8.h"
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <_types/_uint8_t.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <stdio.h>
#include <unistd.h>

void THROW_UNRECOGNISED_OPCODE(uint32_t opcode) {
  printf("Invalid instruction! opcode: 0x%04X\n", opcode);
  exit(1);
}

Chip8::Chip8(char *romFilePath) {
  clearMemory();
  loadROMFileFromPath(romFilePath);
  SP = 0;
}

void Chip8::clearMemory() {
  for (int i = 0; i < 4096; ++i)
    memory[i] = 0;
}

void Chip8::loadROMFileFromPath(char *romFilePath) {
  std::cout << "Reading file: " << romFilePath << std::endl;

  std::fstream romFile(romFilePath);

  if (!romFile.is_open()) {
    std::cerr << "Failed to read file: " << romFilePath << std::endl;
    return;
  }

  this->PC = 0x200;
  while (romFile)
    memory[PC++] = romFile.get();
  this->PC = 0x200;

  romFile.close();

  std::cout << "Loaded file: " << romFilePath << std::endl;
}

void print_opcode(uint16_t opcode) { printf("0x%04X\n", opcode); }

void Chip8::drawDisplayToTerminal() {
  for (int i = 0; i < 32; ++i) {
    std::bitset<64> row(display.bits[i]);
    std::cout << row << '\n';
  }
}

void Chip8::run() {

  const int SCREEN_HEIGHT = 320;
  const int SCREEN_WIDTH = 640;

  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
  else {
    window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == NULL)
      printf("Failed to create SDL window! SDL Error: %s\n", SDL_GetError());
    else {
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

      SDL_Event e;
      bool quit = false;
      while (quit == false) {
        while (SDL_PollEvent(&e)) {
          if (e.type == SDL_QUIT)
            quit = true;
        }

        uint32_t opcode = (memory[PC] << 8) | memory[PC + 1];

        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        uint8_t kk = (opcode & 0x00FF);
        uint16_t nnn = opcode & 0x0FFF;

        printf("%02X : %02X : %04X : %04X\n", PC, SP, stack[SP], opcode);

        // print_opcode(opcode);
        // printf("%d\n", stack[SP]);

        switch (opcode & 0xF000) {

        case 0x0000: {

          // 00EE - RET
          // Return from a subroutine.
          if (opcode == 0x00EE) {

            if (SP == 0) {
              printf("Stack undeflow!\n");
              exit(1);
            }

            PC = stack[SP];
            --SP;
          } else {

            // 00E0 - CLS
            // Clear the display.
            if (opcode == 0x00E0) {
              for (int i = 0; i < 32; ++i)
                display.bits[i] = 0ull;
            }

            PC += 2;
          }

          break;
        }

        // 1nnn - JP addr
        // Jump to location nnn.
        case 0x1000: {
          PC = nnn;
          break;
        }

        // 2nnn - CALL addr
        // Call subroutine at nnn.
        case 0x2000: {
          if (SP == 15) {
            printf("Stack overflow!\n");
            exit(1);
          }

          ++SP;
          stack[SP] = PC + 2;
          PC = nnn;
          break;
        }

        // 3xkk - SE Vx, byte
        // Skip next instruction if Vx = kk.
        case 0x3000: {
          PC += V[x] == kk ? 4 : 2;
          break;
        }

        // 4xkk - SNE Vx, byte
        // Skip next instruction if Vx != kk.
        case 0x4000: {
          PC += V[x] != kk ? 4 : 2;
          break;
        }

        // 5xy0 - SE Vx, Vy
        // Skip next instruction if Vx = Vy.
        case 0x5000: {
          PC += V[x] == V[y] ? 4 : 2;
          break;
        }

        // Dxyn - DRW Vx, Vy, nibble
        // Display n-byte sprite starting at memory location I at (Vx, Vy),
        // set VF = collision.
        case 0xD000: {
          uint8_t n = opcode & 0x000F;

          printf("DRAW x: %d, y: %d, n: %d\n", V[x], V[y], n);

          for (int i = 0; i < n; ++i) {

            uint64_t valueToXOR =
                ((uint64_t)(0ull | memory[I + i]) << 56) >> V[x];

            if (valueToXOR & display.bits[V[y] + i])
              V[0xF] = 1;

            display.bits[V[y] + i] ^= valueToXOR;
          }

          PC += 2;
          break;
        }

        // Annn - LD I, addr
        case 0xA000: {
          I = nnn;
          PC += 2;
          break;
        }

        // 6xkk LD Vx, byte
        case 0x6000: {
          V[x] = kk;
          PC += 2;
          break;
        }

        // 7xkk - ADD Vx, byte
        // Set Vx = Vx + kk.
        case 0x7000: {
          V[x] += kk;
          PC += 2;
          break;
        }

        case 0x8000: {
          switch (opcode & 0x000F) {

          // 8xy0 - LD Vx, Vy
          // Set Vx = Vy.
          case 0: {
            V[x] = V[y];
            PC += 2;
            break;
          }

          // 8xy1 - OR Vx, Vy
          // Set Vx = Vx OR Vy.
          case 1: {
            V[x] |= V[y];
            PC += 2;
            break;
          }

          // 8xy2 - AND Vx, Vy
          // Set Vx = Vx AND Vy.
          case 2: {
            V[x] &= V[y];
            PC += 2;
            break;
          }

          // 8xy3 - XOR Vx, Vy
          // Set Vx = Vx XOR Vy.
          case 3: {
            V[x] ^= V[y];
            PC += 2;
            break;
          }

          // 8xy4 - ADD Vx, Vy
          // Set Vx = Vx + Vy, set VF = carry.
          case 4: {
            V[0xF] = (V[x] + V[y] > 255) ? 1 : 0;
            V[x] += V[y];
            PC += 2;
            break;
          }

          // 8xy5 - SUB Vx, Vy
          // Set Vx = Vx - Vy, set VF = NOT borrow.
          case 5: {
            V[0xF] = V[x] > V[y] ? 1 : 0;
            V[x] -= V[y];
            PC += 2;
            break;
          }

          // 8xy6 - SHR Vx {, Vy}
          // Set Vx = Vx SHR 1.
          case 6: {
            V[x] = V[y];
            V[0xF] = V[x] & 1ull;
            V[x] >>= 1;

            PC += 2;
            break;
          }

          // 8xy7 - SUBN Vx, Vy
          // Set Vx = Vy - Vx, set VF = NOT borrow.
          case 7: {
            V[0xF] = V[y] > V[x] ? 1 : 0;
            V[x] = V[y] - V[x];
            PC += 2;
            break;
          }

          // 8xyE - SHL Vx {, Vy}
          // Set Vx = Vx SHL 1.
          case 0xE: {
            V[x] = V[y];
            V[0xF] = V[x] & (1ull << 8);
            V[x] <<= 1;

            PC += 2;
            break;
          }

          default: {
            THROW_UNRECOGNISED_OPCODE(opcode);
          }
          }

          break;
        }

        // 9xy0 - SNE Vx, Vy
        // Skip next instruction if Vx != Vy.
        case 0x9000: {
          PC += V[x] != V[y] ? 4 : 2;
          break;
        }

        case 0xF000: {
          switch (opcode & 0xF0FF) {

          // Fx07 - LD Vx, DT
          // Set Vx = delay timer value.
          case 0xF007: {
            V[x] = delayTimer;
            PC += 2;
            break;
          }

          // Fx0A - LD Vx, K
          // Wait for a key press, store the value of the key in Vx.
          case 0xF00A: {
            SDL_Event e;
            bool waitingForKey = true;
            while (waitingForKey) {
              if (SDL_WaitEvent(&e)) {
                if (e.type == SDL_KEYDOWN) {
                  switch (e.key.keysym.sym) {

                  case SDLK_1:
                    V[x] = 0x1;
                    waitingForKey = false;
                    break;
                  case SDLK_2:
                    V[x] = 0x2;
                    waitingForKey = false;
                    break;
                  case SDLK_3:
                    V[x] = 0x3;
                    waitingForKey = false;
                    break;
                  case SDLK_4:
                    V[x] = 0xC;
                    waitingForKey = false;
                    break;

                  case SDLK_q:
                    V[x] = 0x4;
                    waitingForKey = false;
                    break;
                  case SDLK_w:
                    V[x] = 0x5;
                    waitingForKey = false;
                    break;
                  case SDLK_e:
                    V[x] = 0x6;
                    waitingForKey = false;
                    break;
                  case SDLK_r:
                    V[x] = 0xD;
                    waitingForKey = false;
                    break;

                  case SDLK_a:
                    V[x] = 0x7;
                    waitingForKey = false;
                    break;
                  case SDLK_s:
                    V[x] = 0x8;
                    waitingForKey = false;
                    break;
                  case SDLK_d:
                    V[x] = 0x9;
                    waitingForKey = false;
                    break;
                  case SDLK_f:
                    V[x] = 0xE;
                    waitingForKey = false;
                    break;

                  case SDLK_z:
                    V[x] = 0xA;
                    waitingForKey = false;
                    break;
                  case SDLK_x:
                    V[x] = 0x0;
                    waitingForKey = false;
                    break;
                  case SDLK_c:
                    V[x] = 0xB;
                    waitingForKey = false;
                    break;
                  case SDLK_v:
                    V[x] = 0xF;
                    waitingForKey = false;
                    break;
                  }
                }
              }
            }

            printf("Key pressed: %01X\n", V[x]);

            PC += 2;
            break;
          }

          // Fx15 - LD DT, Vx
          // Set delay timer = Vx.
          case 0xF015: {
            delayTimer = V[x];
            PC += 2;
            break;
          }

          // Fx18 - LD ST, Vx
          // Set sound timer = Vx.
          case 0xF018: {
            soundTimer = V[x];
            PC += 2;
            break;
          }

          // Fx1E - ADD I, Vx
          // Set I = I + Vx.
          case 0xF01E: {
            I += V[x];
            PC += 2;
            break;
          }

            // Fx29 - LD F, Vx
            // Set I = location of sprite for digit Vx.
            // case 0xF029: {
            //   // TODO
            //   PC += 2;
            //   break;
            // }

          // Fx33 - LD B, Vx
          // Store BCD representation of Vx in memory locations I,
          // I+1, and I+2.
          case 0xF033: {

            uint8_t o = V[x] % 10;
            uint8_t t = ((V[x] % 100) - o) / 10;
            uint8_t h = ((V[x] % 1000) - t - o) / 100;

            memory[I] = h;
            memory[I + 1] = t;
            memory[I + 2] = o;

            PC += 2;
            break;
          }

          // Fx55 - LD [I], Vx
          // Store registers V0 through Vx in memory starting at
          // location I.
          case 0xF055: {
            for (int i = 0; i <= x; ++i) {
              memory[I + i] = V[i];
            }

            PC += 2;
            break;
          }

          // Fx65 - LD Vx, [I]
          // Read registers V0 through Vx from memory starting at
          // location I.
          case 0xF065: {
            for (int i = 0; i <= x; ++i) {
              V[i] = memory[I + i];
            }

            PC += 2;
            break;
          }

          default: {
            THROW_UNRECOGNISED_OPCODE(opcode);
          }
          }

          break;
        }

        default: {
          printf("Invalid instruction! opcode: 0x%04X\n", opcode);
          exit(1);
        }
        }

        // Set render color to red ( background will be rendered in this color )
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear winow
        SDL_RenderClear(renderer);

        for (int displayRow = 0; displayRow < 32; ++displayRow) {
          for (int displayCol = 0; displayCol < 64; ++displayCol) {

            bool isPixelLit =
                ((display.bits[displayRow]) & (1ull << (63 - displayCol))) != 0;

            if (!isPixelLit)
              continue;

            SDL_Rect r;
            r.h = 10;
            r.w = 10;
            r.x = displayCol * 10;
            r.y = displayRow * 10;

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &r);
          }
        }

        SDL_RenderPresent(renderer);
        // usleep(12000);
      }
    }
  }
}
