#include <SDL2/SDL.h>
#include "sand-sim.h"

const int GRAIN_RENDER_SIZE = 10;
const int GRAIN_COUNT_WIDTH = 50;
const int GRAIN_COUNT_HEIGHT = 50;

const int MS_PER_FRAME = 16;

int map_physical_to_sim_coord(int val){
    return val / GRAIN_RENDER_SIZE;
}

void handle_event(SDL_Event event, struct sim_state* s_state, struct button_state* b_state ){
    if(event.type == SDL_QUIT){
        s_state -> quit = 1;
    }
    else if(event.type == SDL_KEYDOWN){
        if(event.key.keysym.sym == SDLK_ESCAPE){
            s_state -> quit = 1;
        }
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN){
        if(event.button.button == SDL_BUTTON_LEFT){
            b_state -> left_mouse_pressed = 1;
        }
        else if(event.button.button == SDL_BUTTON_RIGHT){
            b_state -> right_mouse_pressed = 1;
        }
    }
    else if(event.type == SDL_MOUSEBUTTONUP){
        if(event.button.button == SDL_BUTTON_LEFT){
            b_state -> left_mouse_pressed = 0;
        }
        else if(event.button.button == SDL_BUTTON_RIGHT){
            b_state -> right_mouse_pressed = 0;
        }
    }
    else if(event.type == SDL_MOUSEMOTION){
        b_state -> mouse_x = event.motion.x;
        b_state -> mouse_y = event.motion.y;
    }
}

void handle_input(struct button_state* b_state, uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH]){
    if(b_state -> left_mouse_pressed){
        int sim_row = map_physical_to_sim_coord(b_state -> mouse_y);
        int sim_col = map_physical_to_sim_coord(b_state -> mouse_x);

        if(sim_row >= 0 && sim_row < GRAIN_COUNT_HEIGHT && sim_col >= 0 && sim_col < GRAIN_COUNT_WIDTH){
            sim_space[sim_row][sim_col] = 1;
        }
    }

    if(b_state -> right_mouse_pressed){
        int sim_row = map_physical_to_sim_coord(b_state -> mouse_y);
        int sim_col = map_physical_to_sim_coord(b_state -> mouse_x);
        

        if(sim_row >= 0 && sim_row < GRAIN_COUNT_HEIGHT && sim_col >= 0 && sim_col < GRAIN_COUNT_WIDTH){
            sim_space[sim_row][sim_col] = 0;
        }
    }
}

void move_element(uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH], int row, int col, int target_row, int target_col){
    uint8_t cell_value = sim_space[row][col];
    sim_space[row][col] = 0;
    sim_space[target_row][target_col] = cell_value;
}

void move_element_down(uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH], int row, int col){
    move_element(sim_space, row, col, row+1, col);
}

void move_element_down_left(uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH], int row, int col){
    move_element(sim_space, row, col, row+1, col-1);
}

void move_element_down_right(uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH], int row, int col){
    move_element(sim_space, row, col, row+1, col+1);
}

void do_physics(uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH]){
    // if we started from the first row and scanned down, we would instantly move grains to the bottom of the screen
    // fixing this would require two sim state buffer and swapping between them
    // starting from bottom we dont need to have two buffers as we cant move the same grain twice in one pass
    // dont bother with the bottom row as things cant fall further

    for(int row = GRAIN_COUNT_HEIGHT - 2; row>= 0; row--){
        for(int col = 0; col < GRAIN_COUNT_WIDTH; col++){

            // current element is not empty space
            if( sim_space[row][col] != 0){

                // can move grain straight down
                if( sim_space[row + 1][col] == 0 ){
                    move_element_down(sim_space, row, col);
                }
                else{
                    // no gap below, attempt down left or down right, randomized.
                    int can_move_down_left = ((col - 1 >= 0) && sim_space[row + 1][col - 1] == 0);
                    int can_move_down_right = (( col + 1 < GRAIN_COUNT_WIDTH) && sim_space[row + 1][col + 1] == 0);

                    if ( can_move_down_left && !can_move_down_right ){
                        move_element_down_left(sim_space, row, col);
                    }
                    else if( can_move_down_right && !can_move_down_left ){
                        move_element_down_right(sim_space, row, col);
                    }
                    else if( can_move_down_left && can_move_down_right ){
                        int dir = rand() % 2; // random either 0 or 1. left is 0, right is 1
                        if(dir == 0){
                            move_element_down_left(sim_space, row, col);
                        }
                        else{
                            move_element_down_right(sim_space, row, col);
                        }
                    }
                }
            }
        }
    }
}

void do_render(struct sim_visuals* s_visual, uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH]){
    // make default colour a nice blue
    SDL_SetRenderDrawColor(s_visual -> renderer, 52, 192, 235, 255);
    SDL_RenderClear(s_visual -> renderer);

    // loop over sim space, drawing sand coloured rectangles anywhere than currently has a '1' in it.
    SDL_Rect rect;
    SDL_SetRenderDrawColor(s_visual -> renderer, 214, 208, 141, 255);

    for(int row = 0; row < GRAIN_COUNT_HEIGHT; row++){
        for(int col = 0; col < GRAIN_COUNT_WIDTH; col++){
            if( sim_space[row][col] == 1 ){
                rect.h = GRAIN_RENDER_SIZE;
                rect.w = GRAIN_RENDER_SIZE;
                rect.x = GRAIN_RENDER_SIZE * col;
                rect.y = GRAIN_RENDER_SIZE * row;
                SDL_RenderFillRect(s_visual -> renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(s_visual -> renderer);
}

void SDL_Teardown(struct sim_visuals* s_visual){
    SDL_DestroyTexture(s_visual -> screen_texture);
    SDL_DestroyRenderer(s_visual -> renderer);
    SDL_DestroyWindow(s_visual -> window);
    SDL_Quit();
}

int main(int argc, char** argv){
    int SCREEN_HEIGHT = GRAIN_COUNT_HEIGHT * GRAIN_RENDER_SIZE;
    int SCREEN_WIDTH = GRAIN_COUNT_WIDTH * GRAIN_RENDER_SIZE;

    struct button_state b_state = {
        .mouse_x = 0,
        .mouse_y = 0,
        .left_mouse_pressed = 0,
        .right_mouse_pressed = 0
    };

    struct sim_state s_state = {
        .quit = 0,
    };

    struct sim_visuals s_visual;

    SDL_Event e;

    // extremely space inefficient
    uint8_t sim_space[GRAIN_COUNT_HEIGHT][GRAIN_COUNT_WIDTH];
    memset(sim_space, 0, sizeof(uint8_t) * GRAIN_COUNT_HEIGHT * GRAIN_COUNT_WIDTH);

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf("SDL Init Video failed\n");
        exit(-1);
    }

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &(s_visual.window), &(s_visual.renderer));
    SDL_SetWindowTitle(s_visual.window, "Sand-Sim");
    s_visual.screen_texture = SDL_CreateTexture(s_visual.renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(s_visual.renderer, 0, 0, 0, 255);

    if(s_visual.window == NULL){
        printf("SDL could not create window\n");
        SDL_Teardown(&s_visual);
        exit(-1);
    }

    // render loop
    // - handle events
    // - maybe quit
    // - handle any input event 
    // - do physics update
    // - do rendering
    // - delay until next update
    while( s_state.quit != 1 ){
        Uint64 start_tick = SDL_GetTicks64();

        // may have many events to process
        while(SDL_PollEvent(&e)){
            handle_event(e, &s_state, &b_state);
        }

        if(s_state.quit == 1)
            continue;

        handle_input(&b_state, &sim_space);
        do_physics(&sim_space);
        do_render(&s_visual, &sim_space);

        // burn some time to maintain a reasonable framerate/physics update rate
        if((SDL_GetTicks64() - start_tick) < MS_PER_FRAME) {
            SDL_Delay(MS_PER_FRAME - (SDL_GetTicks64() - start_tick));
        }
        
    }

    SDL_Teardown(&s_visual);
    return 0;
}