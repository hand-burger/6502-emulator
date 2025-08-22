#include "nes_frontend.h"
#include <iostream>

// Simplified NES color palette
const SDL_Color NESFrontend::NES_PALETTE[64] = {
    {84, 84, 84, 255}, {0, 30, 116, 255}, {8, 16, 144, 255}, {48, 0, 136, 255},
    {68, 0, 100, 255}, {92, 0, 48, 255}, {84, 4, 0, 255}, {60, 24, 0, 255},
    {32, 42, 0, 255}, {8, 58, 0, 255}, {0, 64, 0, 255}, {0, 60, 0, 255},
    {0, 50, 60, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255},
    
    {152, 150, 152, 255}, {8, 76, 196, 255}, {48, 50, 236, 255}, {92, 30, 228, 255},
    {136, 20, 176, 255}, {160, 20, 100, 255}, {152, 34, 32, 255}, {120, 60, 0, 255},
    {84, 90, 0, 255}, {40, 114, 0, 255}, {8, 124, 0, 255}, {0, 118, 40, 255},
    {0, 102, 120, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255},
    
    {236, 238, 236, 255}, {76, 154, 236, 255}, {120, 124, 236, 255}, {176, 98, 236, 255},
    {228, 84, 236, 255}, {236, 88, 180, 255}, {236, 106, 100, 255}, {212, 136, 32, 255},
    {160, 170, 0, 255}, {116, 196, 0, 255}, {76, 208, 32, 255}, {56, 204, 108, 255},
    {56, 180, 204, 255}, {60, 60, 60, 255}, {0, 0, 0, 255}, {0, 0, 0, 255},
    
    {236, 238, 236, 255}, {168, 204, 236, 255}, {188, 188, 236, 255}, {212, 178, 236, 255},
    {236, 174, 236, 255}, {236, 174, 212, 255}, {236, 180, 176, 255}, {228, 196, 144, 255},
    {204, 210, 120, 255}, {180, 222, 120, 255}, {168, 226, 144, 255}, {152, 226, 180, 255},
    {160, 214, 228, 255}, {160, 162, 160, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}
};

NESFrontend::NESFrontend() : m_window(nullptr), m_renderer(nullptr), m_texture(nullptr) {}

NESFrontend::~NESFrontend() {
    shutdown();
}

bool NESFrontend::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return false;
    }
    
    m_window = SDL_CreateWindow(
        "NES Emulator", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        NES_WIDTH * SCALE, 
        NES_HEIGHT * SCALE, 
        SDL_WINDOW_SHOWN
    );
    
    if (!m_window) {
        std::fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return false;
    }
    
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        return false;
    }
    
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, NES_HEIGHT);
    if (!m_texture) {
        std::fprintf(stderr, "SDL_CreateTexture error: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

void NESFrontend::shutdown() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void NESFrontend::render(const uint8_t* frameBuffer) {
    void* pixels = nullptr;
    int pitch = 0;
    
    if (SDL_LockTexture(m_texture, nullptr, &pixels, &pitch) != 0) {
        return;
    }
    
    uint8_t* pixelBytes = static_cast<uint8_t*>(pixels);
    
    for (int y = 0; y < NES_HEIGHT; ++y) {
        for (int x = 0; x < NES_WIDTH; ++x) {
            uint8_t colorIndex = frameBuffer[y * NES_WIDTH + x] & 0x3F; // Limit to 64 colors
            const SDL_Color& color = NES_PALETTE[colorIndex];
            
            int pixelIndex = y * pitch + x * 3;
            pixelBytes[pixelIndex + 0] = color.r;
            pixelBytes[pixelIndex + 1] = color.g;
            pixelBytes[pixelIndex + 2] = color.b;
        }
    }
    
    SDL_UnlockTexture(m_texture);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

bool NESFrontend::handleEvents(NES& nes) {
    SDL_Event event;
    static uint8_t controller = 0; // Maintain controller state
    
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            bool pressed = (event.type == SDL_KEYDOWN);
            
            // Map keys to NES controller buttons
            // Bit layout: A, B, Select, Start, Up, Down, Left, Right
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    return false;
                case SDLK_z:      // A button
                    if (pressed) controller |= 0x01; else controller &= ~0x01;
                    break;
                case SDLK_x:      // B button
                    if (pressed) controller |= 0x02; else controller &= ~0x02;
                    break;
                case SDLK_TAB:    // Select
                    if (pressed) controller |= 0x04; else controller &= ~0x04;
                    break;
                case SDLK_RETURN: // Start
                    if (pressed) controller |= 0x08; else controller &= ~0x08;
                    break;
                case SDLK_UP:
                    if (pressed) controller |= 0x10; else controller &= ~0x10;
                    break;
                case SDLK_DOWN:
                    if (pressed) controller |= 0x20; else controller &= ~0x20;
                    break;
                case SDLK_LEFT:
                    if (pressed) controller |= 0x40; else controller &= ~0x40;
                    break;
                case SDLK_RIGHT:
                    if (pressed) controller |= 0x80; else controller &= ~0x80;
                    break;
            }
            
            nes.setControllerState(0, controller);
        }
    }
    
    return true;
}