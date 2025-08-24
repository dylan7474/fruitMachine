#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <stdlib.h>

#define NUM_REELS 3
#define NUM_SYMBOLS 10

const char *symbol_files[NUM_SYMBOLS] = {
    "icon-apple.jpeg",
    "icon-banana.jpeg",
    "icon-cherry.jpeg",
    "icon-grape.jpeg",
    "icon-lemon.jpeg",
    "icon-orange.jpeg",
    "icon-pear.jpeg",
    "icon-pineapple.jpeg",
    "icon-strawberry.jpeg",
    "icon-watermelon.jpeg"
};

typedef struct {
    int spinning;
    Uint32 stop_time;
    float offset;
    int prev;
    int current;
    int next;
} Reel;

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "Fruit Machine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1368,
        768,
        SDL_WINDOW_FULLSCREEN
    );
    if (!win) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture *background = IMG_LoadTexture(ren, "FruitMachineBody.jpeg");
    if (!background) {
        SDL_Log("Failed to load background: %s", IMG_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int win_w, win_h;
    SDL_GetWindowSize(win, &win_w, &win_h);

    SDL_Texture *symbols[NUM_SYMBOLS];
    for (int i = 0; i < NUM_SYMBOLS; ++i) {
        symbols[i] = IMG_LoadTexture(ren, symbol_files[i]);
        if (!symbols[i]) {
            SDL_Log("Failed to load symbol %s: %s", symbol_files[i], IMG_GetError());
            for (int j = 0; j < i; ++j) SDL_DestroyTexture(symbols[j]);
            SDL_DestroyTexture(background);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    srand((unsigned)time(NULL));

    Reel reels[NUM_REELS] = {0};
    for (int i = 0; i < NUM_REELS; ++i) {
        reels[i].prev = rand() % NUM_SYMBOLS;
        reels[i].current = rand() % NUM_SYMBOLS;
        reels[i].next = rand() % NUM_SYMBOLS;
        reels[i].offset = 0;
    }

    /*
     * Reel placement tuned to line up with the windows on the background
     * artwork.  The numbers below are ratios of the overall window size so
     * the layout still scales if the window dimensions change.
     */
    const float reel_centers[NUM_REELS] = {0.26f, 0.5f, 0.74f};
    const float reel_y_center = 0.42f;
    const float reel_width_ratio = 0.14f;
    const float reel_height_ratio = 0.25f;

    int reel_w = (int)(win_w * reel_width_ratio);
    int reel_h = (int)(win_h * reel_height_ratio);
    int spin_speed = reel_h / 8;

    SDL_Rect reel_rects[NUM_REELS];
    for (int i = 0; i < NUM_REELS; ++i) {
        reel_rects[i].w = reel_w;
        reel_rects[i].h = reel_h;
        reel_rects[i].x = (int)(win_w * reel_centers[i] - reel_w / 2);
        reel_rects[i].y = (int)(win_h * reel_y_center - reel_h / 2);
    }

    SDL_Rect start_button = {
        win_w / 2 - reel_w,
        reel_rects[0].y + reel_h + 20,
        reel_w * 2,
        reel_h / 2
    };

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                if (mx >= start_button.x && mx < start_button.x + start_button.w &&
                    my >= start_button.y && my < start_button.y + start_button.h) {
                    for (int i = 0; i < NUM_REELS; ++i) {
                        reels[i].spinning = 1;
                        reels[i].stop_time = SDL_GetTicks() + 500 + i * 500;
                        reels[i].offset = 0;
                        reels[i].prev = rand() % NUM_SYMBOLS;
                        reels[i].current = rand() % NUM_SYMBOLS;
                        reels[i].next = rand() % NUM_SYMBOLS;
                    }
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        for (int i = 0; i < NUM_REELS; ++i) {
            if (reels[i].spinning || reels[i].offset > 0) {
                reels[i].offset += spin_speed;
                if (reels[i].offset >= reel_h) {
                    reels[i].offset -= reel_h;
                    reels[i].prev = reels[i].current;
                    reels[i].current = reels[i].next;
                    if (reels[i].spinning) {
                        reels[i].next = rand() % NUM_SYMBOLS;
                    } else {
                        reels[i].offset = 0;
                    }
                }
                if (reels[i].spinning && now >= reels[i].stop_time) {
                    reels[i].spinning = 0;
                }
            }
        }

        SDL_RenderClear(ren);
        SDL_Rect bg_rect = {0, 0, win_w, win_h};
        SDL_RenderCopy(ren, background, NULL, &bg_rect);
        for (int i = 0; i < NUM_REELS; ++i) {
            SDL_RenderSetClipRect(ren, &reel_rects[i]);
            SDL_Rect dest = {reel_rects[i].x,
                             reel_rects[i].y - reel_h + (int)reels[i].offset,
                             reel_w,
                             reel_h};
            SDL_RenderCopy(ren, symbols[reels[i].prev], NULL, &dest);
            dest.y = reel_rects[i].y + (int)reels[i].offset;
            SDL_RenderCopy(ren, symbols[reels[i].current], NULL, &dest);
            dest.y = reel_rects[i].y + reel_h + (int)reels[i].offset;
            SDL_RenderCopy(ren, symbols[reels[i].next], NULL, &dest);
            SDL_RenderSetClipRect(ren, NULL);
        }
        SDL_RenderPresent(ren);
    }

    for (int i = 0; i < NUM_SYMBOLS; ++i) {
        SDL_DestroyTexture(symbols[i]);
    }
    SDL_DestroyTexture(background);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
