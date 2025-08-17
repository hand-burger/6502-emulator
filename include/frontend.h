#ifndef FRONTEND_H
#define FRONTEND_H

#include "cpu.h"
#include <SDL.h>
#include <vector>

class Frontend {
public:
    Frontend();
    ~Frontend();

    bool init();
    void shutdown();
    void draw_if_changed(const Byte* screen);
    bool handle_events(cpu& cpu);

private:
    static constexpr int W = 32;
    static constexpr int H = 32;
    static constexpr int SCALE = 16;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    std::vector<Byte> last_frame;

    static inline void map_to_rgb(Byte v, Byte& r, Byte& g, Byte& b);
};

#endif // FRONTEND_H
