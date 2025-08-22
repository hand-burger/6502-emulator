#ifndef NES_FRONTEND_H
#define NES_FRONTEND_H

#include "nes.h"
#include <SDL.h>
#include <vector>

class NESFrontend {
public:
    NESFrontend();
    ~NESFrontend();

    bool init();
    void shutdown();
    void render(const uint8_t* frameBuffer);
    bool handleEvents(NES& nes);

private:
    static constexpr int NES_WIDTH = 256;
    static constexpr int NES_HEIGHT = 240;
    static constexpr int SCALE = 3;

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;

    // NES color palette (simplified)
    static const SDL_Color NES_PALETTE[64];
    
    void drawPixel(int x, int y, uint8_t colorIndex);
};

#endif // NES_FRONTEND_H