#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

struct button_state {
    int left_mouse_pressed;
    int clear_key_pressed;
    int mouse_x;
    int mouse_y;
};

struct sim_visuals {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;
};

struct sim_state {
    int current_row;
    int generate_new_rows;
    int quit;
};