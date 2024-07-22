#include "chip8.h"

#include <SDL.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

void THROW_UNRECOGNISED_OPCODE(uint32_t opcode)
{
    printf("Invalid instruction! opcode: 0x%04X\n", opcode);
    exit(1);
}

Chip8::Chip8(char* romFilePath)
{
    clearMemory();
    loadROMFileFromPath(romFilePath);
    SP = 0;
    delayTimer = 0;
    for (int i = 0; i < 16; ++i)
        keyboard[i] = false;
}

void Chip8::clearMemory()
{
    for (int i = 0; i < 4096; ++i)
        memory[i] = 0;

    for (int spriteIdx = 0; spriteIdx < 16; ++spriteIdx) {
        for (int spriteLineIdx = 0; spriteLineIdx < 5; ++spriteLineIdx) {
            memory[spriteIdx * 5 + spriteLineIdx]
                = NUMBER_SPRITES[spriteIdx][spriteLineIdx];
        }
    }
}

void Chip8::loadROMFileFromPath(char* romFilePath)
{
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

void Chip8::drawDisplayToTerminal()
{
    for (int i = 0; i < 32; ++i) {
        std::bitset<64> row(display[1].bits[i]);
        std::cout << row << '\n';
    }
}

void Chip8::run()
{
    const int SCREEN_HEIGHT = 320;
    const int SCREEN_WIDTH = 640;

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    std::vector<SDL_Rect> rectangles;
    rectangles.reserve(2048);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    else {
        window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);

        if (window == NULL)
            printf(
                "Failed to create SDL window! SDL Error: %s\n", SDL_GetError());
        else {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

            SDL_Event e;
            bool quit = false;

            interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                           .count();

            auto clock_interval = std::chrono::high_resolution_clock::now();
            uint64_t seconds_interval = interval;

            uint64_t instructions_executed = 0;
            uint64_t frames_rendered = 0;

            while (quit == false) {
                while (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT)
                        quit = true;

                    if (e.type == SDL_KEYUP || e.type == SDL_KEYDOWN) {
                        bool isKeyPressed
                            = e.type == SDL_KEYDOWN && e.type != SDL_KEYUP;

                        switch (e.key.keysym.sym) {
                        case SDLK_1:
                            keyboard[1] = isKeyPressed;
                            break;
                        case SDLK_2:
                            keyboard[2] = isKeyPressed;
                            break;
                        case SDLK_3:
                            keyboard[3] = isKeyPressed;
                            break;
                        case SDLK_4:
                            keyboard[0xC] = isKeyPressed;
                            break;

                        case SDLK_q:
                            keyboard[4] = isKeyPressed;
                            break;
                        case SDLK_w:
                            keyboard[5] = isKeyPressed;
                            break;
                        case SDLK_e:
                            keyboard[6] = isKeyPressed;
                            break;
                        case SDLK_r:
                            keyboard[0xD] = isKeyPressed;
                            break;

                        case SDLK_a:
                            keyboard[7] = isKeyPressed;
                            break;
                        case SDLK_s:
                            keyboard[8] = isKeyPressed;
                            break;
                        case SDLK_d:
                            keyboard[9] = isKeyPressed;
                            break;
                        case SDLK_f:
                            keyboard[0xE] = isKeyPressed;
                            break;

                        case SDLK_z:
                            keyboard[0xA] = isKeyPressed;
                            break;
                        case SDLK_x:
                            keyboard[0] = isKeyPressed;
                            break;
                        case SDLK_c:
                            keyboard[0xB] = isKeyPressed;
                            break;
                        case SDLK_v:
                            keyboard[0xF] = isKeyPressed;
                            break;
                        }
                    }
                }

                uint32_t opcode = (memory[PC] << 8) | memory[PC + 1];

                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                uint8_t kk = (opcode & 0x00FF);
                uint16_t nnn = opcode & 0x0FFF;

                uint64_t now
                    = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                          .count();

                const std::chrono::duration<double> diff
                    = std::chrono::high_resolution_clock::now()
                    - clock_interval;

                if (diff.count() >= 0.001666f) {
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
                                memcpy(display[0].bits, display[1].bits,
                                    32 * sizeof(uint64_t));
                                for (int i = 0; i < 32; ++i)
                                    display[1].bits[i] = 0ull;
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

                    // Cxkk - RND Vx, byte
                    // Set Vx = random byte AND kk.
                    case 0xC000: {
                        uint8_t random = rand() % 255;
                        V[x] = random & kk;
                        PC += 2;
                        break;
                    }

                    // Dxyn - DRW Vx, Vy, nibble
                    // Display n-byte sprite starting at memory location I at
                    // (Vx, Vy), set VF = collision.
                    case 0xD000: {
                        uint8_t n = opcode & 0x000F;

                        memcpy(display[0].bits, display[1].bits,
                            32 * sizeof(uint64_t));

                        // printf("DRAW x: %d, y: %d, n: %d\n", V[x], V[y], n);

                        V[0xF] = 0;

                        int X = V[x] % 64;
                        int Y = V[y] % 32;

                        for (int i = 0; i < n; ++i) {
                            uint64_t valueToXOR
                                = ((uint64_t)(0ull | memory[I + i]) << 56) >> X;

                            if (valueToXOR & display[1].bits[(Y + i) % 32])
                                V[0xF] = 1;

                            display[1].bits[Y + i] ^= valueToXOR;
                        }

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
                            V[0xF] = 0;
                            PC += 2;
                            break;
                        }

                        // 8xy2 - AND Vx, Vy
                        // Set Vx = Vx AND Vy.
                        case 2: {
                            V[x] &= V[y];
                            V[0xF] = 0;
                            PC += 2;
                            break;
                        }

                        // 8xy3 - XOR Vx, Vy
                        // Set Vx = Vx XOR Vy.
                        case 3: {
                            V[x] ^= V[y];
                            V[0xF] = 0;
                            PC += 2;
                            break;
                        }

                        // 8xy4 - ADD Vx, Vy
                        // Set Vx = Vx + Vy, set VF = carry.
                        case 4: {
                            uint8_t carry = (V[x] + V[y] > 255) ? 1 : 0;
                            V[x] += V[y];
                            V[0xF] = carry;
                            PC += 2;
                            break;
                        }

                        // 8xy5 - SUB Vx, Vy
                        // Set Vx = Vx - Vy, set VF = NOT borrow.
                        case 5: {
                            uint8_t notBorrow = V[x] > V[y] ? 1 : 0;
                            V[x] -= V[y];
                            V[0xF] = notBorrow;
                            PC += 2;
                            break;
                        }

                        // 8xy6 - SHR Vx {, Vy}
                        // Set Vx = Vx SHR 1.
                        case 6: {
                            uint8_t cutoffBit = V[x] & 1;
                            V[x] = V[y];
                            V[x] >>= 1;
                            V[0xF] = cutoffBit;
                            PC += 2;
                            break;
                        }

                        // 8xy7 - SUBN Vx, Vy
                        // Set Vx = Vy - Vx, set VF = NOT borrow.
                        case 7: {
                            uint8_t notBorrow = V[y] > V[x] ? 1 : 0;
                            V[x] = V[y] - V[x];
                            V[0xF] = notBorrow;
                            PC += 2;
                            break;
                        }

                        // 8xyE - SHL Vx {, Vy}
                        // Set Vx = Vx SHL 1.
                        case 0xE: {
                            uint8_t cutoffBit = V[x] >> 7;
                            V[x] = V[y];
                            V[x] <<= 1;
                            V[0xF] = cutoffBit;
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

                    // Annn - LD I, addr
                    case 0xA000: {
                        I = nnn;
                        PC += 2;
                        break;
                    }

                    // Bnnn - JP V0, addr
                    // Jump to location nnn + V0.
                    case 0xB000: {
                        PC = nnn + V[0];
                        break;
                    }

                    case 0xE000: {
                        switch (opcode & 0x00FF) {
                        // Ex9E - SKP Vx
                        // Skip next instruction if key with the value of
                        // Vx is pressed.
                        case 0x009E: {
                            if (keyboard[V[x]]) {
                                PC += 2;
                            }
                            PC += 2;
                            break;
                        }

                        // ExA1 - SKNP Vx
                        // Skip next instruction if key with the value of
                        // Vx is not pressed.
                        case 0x00A1: {
                            if (!keyboard[V[x]]) {
                                PC += 2;
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
                        // Wait for a key press, store the value of the key
                        // in Vx.
                        case 0xF00A: {
                            for (int i = 0; i < 16; ++i) {
                                if (keyboard[i] == true) {
                                    V[x] = i;
                                    printf("Key pressed: %01X\n", V[x]);
                                    PC += 2;

                                    break;
                                }
                            }

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
                        case 0xF029: {
                            I = x * 5;
                            PC += 2;
                            break;
                        }

                        // Fx33 - LD B, Vx
                        // Store BCD representation of Vx in memory
                        // locations I, I+1, and I+2.
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
                        // Store registers V0 through Vx in memory starting
                        // at location I.
                        case 0xF055: {
                            for (int i = 0; i <= x; ++i) {
                                memory[I + i] = V[i];
                            }
                            I += x + 1;

                            PC += 2;
                            break;
                        }

                        // Fx65 - LD Vx, [I]
                        // Read registers V0 through Vx from memory
                        // starting at location I.
                        case 0xF065: {
                            for (int i = 0; i <= x; ++i) {
                                V[i] = memory[I + i];
                            }
                            I += x + 1;

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

                    ++instructions_executed;
                    clock_interval = std::chrono::high_resolution_clock::now();
                }

                if (now - interval >= 17) {
                    if (delayTimer > 0)
                        --delayTimer;

                    ++frames_rendered;
                    interval = now;

                    rectangles.clear();

                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

                    for (int displayRow = 0; displayRow < 32; ++displayRow) {
                        for (int displayCol = 0; displayCol < 64;
                             ++displayCol) {
                            bool isPixelLit
                                = ((display[1].bits[displayRow]
                                       | display[0].bits[displayRow])
                                      & (1ull << (63 - displayCol)))
                                != 0;

                            if (!isPixelLit)
                                continue;

                            SDL_Rect r;
                            r.h = 10;
                            r.w = 10;
                            r.x = displayCol * 10;
                            r.y = displayRow * 10;

                            rectangles.push_back(r);
                        }
                    }

                    SDL_RenderFillRects(
                        renderer, rectangles.data(), rectangles.size());
                    SDL_RenderPresent(renderer);
                }

                if (now - seconds_interval >= 1000) {
                    printf("%d ", delayTimer);
                    std::cout << instructions_executed << "Hz "
                              << " " << frames_rendered << "fps" << std::endl;

                    frames_rendered = 0;

                    seconds_interval = now;
                    instructions_executed = 0;

                    if (delayTimer > 0) {
                        --delayTimer;
                    }
                }
            }
        }
    }
}
