#include <SDL2/SDL.h>
#include "one-dimension-automaton.h"

const int PIXEL_RENDER_SIZE = 30;
const int PIXEL_COUNT_WIDTH = 40;
const int PIXEL_COUNT_HEIGHT = 16;

const int MS_PER_FRAME = 16;

const int RULE_TO_APPLY = 22;

int map_physical_to_sim_coord(int val){
    return val / PIXEL_RENDER_SIZE;
}

int apply_rule(int rule, int input){
    // shift rule right by input number of places, extract last bit
    return (rule >> input) & 1;
}

void handle_event(SDL_Event event, struct sim_state* s_state, struct button_state* b_state ){
    if(event.type == SDL_QUIT){
        s_state -> quit = 1;
    }
    else if(event.type == SDL_KEYDOWN){
        if(event.key.keysym.sym == SDLK_ESCAPE){
            s_state -> quit = 1;
        }
        if(event.key.keysym.sym == SDLK_RETURN){
            s_state -> generate_new_rows = 1;
        }
        if(event.key.keysym.sym == SDLK_c){
            b_state -> clear_key_pressed = 1;
        }
    }
    else if(event.type == SDL_KEYUP){
        if(event.key.keysym.sym == SDLK_c){
            b_state -> clear_key_pressed = 0;
        }
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN){
        if(event.button.button == SDL_BUTTON_LEFT){
            b_state -> left_mouse_pressed = 1;
        }
    }
    else if(event.type == SDL_MOUSEBUTTONUP){
        if(event.button.button == SDL_BUTTON_LEFT){
            b_state -> left_mouse_pressed = 0;
        }
    }
    else if(event.type == SDL_MOUSEMOTION){
        b_state -> mouse_x = event.motion.x;
        b_state -> mouse_y = event.motion.y;
    }
}

void handle_input(struct button_state* b_state, struct sim_state* s_state, uint8_t sim_space[PIXEL_COUNT_HEIGHT][PIXEL_COUNT_WIDTH]){
    // only register inputs on row zero
    if(b_state -> left_mouse_pressed){
        int sim_row = map_physical_to_sim_coord(b_state -> mouse_y);
        int sim_col = map_physical_to_sim_coord(b_state -> mouse_x);

        if(sim_row == 0 && sim_col >= 0 && sim_col <= (PIXEL_COUNT_WIDTH - 1)){
            sim_space[0][sim_col] = 1;
        }
    }

    if(b_state -> clear_key_pressed){
        s_state -> generate_new_rows = 0;
        s_state -> current_row = 0;
        memset(sim_space, 0, sizeof(uint8_t) * PIXEL_COUNT_HEIGHT * PIXEL_COUNT_WIDTH);
    }
}

void do_update(uint8_t sim_space[PIXEL_COUNT_HEIGHT][PIXEL_COUNT_WIDTH], struct sim_state* s_state){
    // only generate one row per update cycle because it's more interesting to look at
    if(s_state->generate_new_rows && s_state->current_row < PIXEL_COUNT_HEIGHT - 1){
        // sliding window over current row, centered
        for (int i = 0; i < PIXEL_COUNT_WIDTH; i++){
            // get i-1, i, i+1, and concatenate them to make a 3 bit number
            int total = 0;

            // first bit
            if(i != 0){
                // maybe populate it with left if possible.
                // otherwise fine to leave as zero.
                total = total | sim_space[s_state->current_row][i-1];
            }

            // second bit
            total = total << 1;
            total = total | sim_space[s_state->current_row][i];
            total = total << 1;

            // third bit
            if(i != (PIXEL_COUNT_WIDTH - 1)){
                // maybe populate it with right if possible. 
                // Otherwise fine to leave as zero
                total = total | sim_space[s_state->current_row][i+1];
            }

            // map that total to an output state and put it in the row below
            sim_space[s_state->current_row + 1][i] = apply_rule(RULE_TO_APPLY, total);
        }

        s_state->current_row++;
    }
}

void do_render(struct sim_visuals* s_visual, uint8_t sim_space[PIXEL_COUNT_HEIGHT][PIXEL_COUNT_WIDTH]){
    // set background colour
    SDL_SetRenderDrawColor(s_visual -> renderer, 158, 217, 213, 255);
    SDL_RenderClear(s_visual -> renderer);

    // loop over sim space, drawing sand coloured rectangles anywhere than currently has a '1' in it.
    SDL_Rect rect;
    SDL_SetRenderDrawColor(s_visual -> renderer, 156, 0, 83, 255);

    for(int row = 0; row < PIXEL_COUNT_HEIGHT; row++){
        for(int col = 0; col < PIXEL_COUNT_WIDTH; col++){
            if( sim_space[row][col] == 1 ){
                rect.h = PIXEL_RENDER_SIZE;
                rect.w = PIXEL_RENDER_SIZE;
                rect.x = PIXEL_RENDER_SIZE * col;
                rect.y = PIXEL_RENDER_SIZE * row;
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
    int SCREEN_HEIGHT = PIXEL_COUNT_HEIGHT * PIXEL_RENDER_SIZE;
    int SCREEN_WIDTH = PIXEL_COUNT_WIDTH * PIXEL_RENDER_SIZE;

    struct button_state b_state = {
        .mouse_x = 0,
        .mouse_y = 0,
        .left_mouse_pressed = 0,
    };

    struct sim_state s_state = {
        .quit = 0,
        .current_row = 0,
        .generate_new_rows = 0
    };

    struct sim_visuals s_visual;

    SDL_Event e;

    // extremely space inefficient
    uint8_t sim_space[PIXEL_COUNT_HEIGHT][PIXEL_COUNT_WIDTH];
    memset(sim_space, 0, sizeof(uint8_t) * PIXEL_COUNT_HEIGHT * PIXEL_COUNT_WIDTH);

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf("SDL Init Video failed\n");
        exit(-1);
    }

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &(s_visual.window), &(s_visual.renderer));
    SDL_SetWindowTitle(s_visual.window, "1D Automaton");
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

        handle_input(&b_state, &s_state, &sim_space);
        do_update(&sim_space, &s_state);
        do_render(&s_visual, &sim_space);

        // burn some time to maintain a reasonable framerate/physics update rate
        if((SDL_GetTicks64() - start_tick) < MS_PER_FRAME) {
            SDL_Delay(MS_PER_FRAME - (SDL_GetTicks64() - start_tick));
        }
        
    }

    SDL_Teardown(&s_visual);
    return 0;
}