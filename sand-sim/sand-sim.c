#include <SDL2/SDL.h>

const int GRAIN_RENDER_SIZE = 10;
const int GRAIN_COUNT_WIDTH = 50;
const int GRAIN_COUNT_HEIGHT = 50;

const int MS_PER_FRAME = 33;

struct button_state {
    int left_mouse_pressed;
    int right_mouse_pressed;
    int mouse_x;
    int mouse_y;
};

struct sim_state {
    int quit;
};

int map_physical_to_sim_coord(int val){
    return val / GRAIN_RENDER_SIZE;
}

int main(int argc, char** argv){
    int SCREEN_HEIGHT = GRAIN_COUNT_HEIGHT * GRAIN_RENDER_SIZE;
    int SCREEN_WIDTH = GRAIN_COUNT_WIDTH * GRAIN_RENDER_SIZE;

    struct button_state b_state;
    struct sim_state s_state;

    b_state.left_mouse_pressed = 0;
    b_state.right_mouse_pressed = 0;
    b_state.mouse_x = 0;
    b_state.mouse_y = 0;

    s_state.quit = 0;

    // extremely space inefficient
    uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH];
    memset(sim_space, 0, sizeof(uint8_t) * GRAIN_COUNT_HEIGHT * GRAIN_COUNT_WIDTH);

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf("SDL Init Video failed\n");
        exit(-1);
    }

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowTitle(window, "Sand-Sim");
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    if(window == NULL){
        printf("SDL could not create window\n");
        SDL_Quit();
        exit(-1);
    }

    SDL_Event e;

    // render loop
    // - check for events
    //   - check for exit events
    //   - check for mousedown events
    // - update sim state
    // - render
    while( s_state.quit != 1 ){
        // loop start time
        Uint64 start_tick = SDL_GetTicks64();

        // handle SDL events
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                s_state.quit = 1;
            }
            else if(e.type == SDL_KEYDOWN){
                if(e.key.keysym.sym == SDLK_ESCAPE){
                    s_state.quit = 1;
                }
            }
            else if(e.type == SDL_MOUSEBUTTONDOWN){
                if(e.button.button == SDL_BUTTON_LEFT){
                    b_state.left_mouse_pressed = 1;
                }
                else if(e.button.button == SDL_BUTTON_RIGHT){
                    b_state.right_mouse_pressed = 1;
                }
            }
            else if(e.type == SDL_MOUSEBUTTONUP){
                if(e.button.button == SDL_BUTTON_LEFT){
                    b_state.left_mouse_pressed = 0;
                }
                else if(e.button.button == SDL_BUTTON_RIGHT){
                    b_state.right_mouse_pressed = 0;
                }
            }
            else if(e.type == SDL_MOUSEMOTION){
                b_state.mouse_x = e.motion.x;
                b_state.mouse_y = e.motion.y;
            }
        }

        SDL_SetRenderDrawColor(renderer, 52, 192, 235, 255);
        SDL_RenderClear(renderer);

        if(b_state.left_mouse_pressed){
            int sim_row = map_physical_to_sim_coord(b_state.mouse_y);
            int sim_col = map_physical_to_sim_coord(b_state.mouse_x);
            

            if(sim_row >= 0 && sim_row < GRAIN_COUNT_HEIGHT && sim_col >= 0 && sim_col < GRAIN_COUNT_WIDTH){
                sim_space[sim_row][sim_col] = 1;
            }
        }

        if(b_state.right_mouse_pressed){
            int sim_row = map_physical_to_sim_coord(b_state.mouse_y);
            int sim_col = map_physical_to_sim_coord(b_state.mouse_x);
            

            if(sim_row >= 0 && sim_row < GRAIN_COUNT_HEIGHT && sim_col >= 0 && sim_col < GRAIN_COUNT_WIDTH){
                sim_space[sim_row][sim_col] = 0;
            }
        }

        // do some physics
        // start from bottom so we dont need to have two buffers
        // dont bother with the bottom row as things cant fall further
        for(int row = GRAIN_COUNT_HEIGHT - 2; row>= 0; row--){
            for(int col = 0; col < GRAIN_COUNT_WIDTH; col++){
                if( sim_space[row][col] != 0){
                    // gap below
                    if( sim_space[row + 1][col] == 0 ){
                        uint8_t cell_value = sim_space[row][col];
                        sim_space[row][col] = 0;
                        sim_space[row + 1][col] = cell_value;
                    }
                    else{
                        // no gap below, check left or right, randomized.
                        int dir = rand() % 2; // random either 0 or 1. left is 0, right is 1
                        if(dir == 0){
                            // check left then right
                            if((col - 1 >= 0) && sim_space[row + 1][col - 1] == 0){
                                uint8_t cell_value = sim_space[row][col];
                                sim_space[row][col] = 0;
                                sim_space[row + 1][col - 1] = cell_value;
                            }
                            else if(( col + 1 < GRAIN_COUNT_WIDTH) && sim_space[row + 1][col + 1] == 0){
                                uint8_t cell_value = sim_space[row][col];
                                sim_space[row][col] = 0;
                                sim_space[row + 1][col + 1] = cell_value;
                            }
                        }
                        else{
                            // check right then left
                            if(( col + 1 < GRAIN_COUNT_WIDTH) && sim_space[row + 1][col + 1] == 0){
                                uint8_t cell_value = sim_space[row][col];
                                sim_space[row][col] = 0;
                                sim_space[row + 1][col + 1] = cell_value;
                            }
                            else if((col - 1 >= 0) && sim_space[row + 1][col - 1] == 0){
                                uint8_t cell_value = sim_space[row][col];
                                sim_space[row][col] = 0;
                                sim_space[row + 1][col - 1] = cell_value;
                            }
                        }
                    }
                    
                }
            }
        }



        // loop over sim space to do rendering
        SDL_Rect rect;
        SDL_SetRenderDrawColor(renderer, 214, 208, 141, 255);
        for(int row = 0; row < GRAIN_COUNT_HEIGHT; row++){
            for(int col = 0; col < GRAIN_COUNT_WIDTH; col++){
                if( sim_space[row][col] == 1 ){
                    rect.h = GRAIN_RENDER_SIZE;
                    rect.w = GRAIN_RENDER_SIZE;
                    rect.x = GRAIN_RENDER_SIZE * col;
                    rect.y = GRAIN_RENDER_SIZE * row;
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        
        // SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // burn enough time for a consistent framerate
        if((SDL_GetTicks64() - start_tick) < MS_PER_FRAME) {
            SDL_Delay(MS_PER_FRAME - (SDL_GetTicks64() - start_tick));
        }
        
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}