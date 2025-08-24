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
    Uint32 next_switch;
    int current;
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

    SDL_Window *win = SDL_CreateWindow("Fruit Machine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
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

    int bg_w, bg_h;
    SDL_QueryTexture(background, NULL, NULL, &bg_w, &bg_h);
    SDL_SetWindowSize(win, bg_w, bg_h);

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
    int reel_w = bg_w / 5;
    int reel_h = bg_h / 3;
    int spacing = reel_w / 2;
    int start_x = (bg_w - (NUM_REELS * reel_w + (NUM_REELS - 1) * spacing)) / 2;
    int slot_y = bg_h / 3 - reel_h / 2;

    SDL_Rect reel_rects[NUM_REELS];
    for (int i = 0; i < NUM_REELS; ++i) {
        reel_rects[i].x = start_x + i * (reel_w + spacing);
        reel_rects[i].y = slot_y;
        reel_rects[i].w = reel_w;
        reel_rects[i].h = reel_h;
    }

    SDL_Rect start_button = {
        bg_w / 2 - reel_w,
        slot_y + reel_h + 20,
        reel_w * 2,
        reel_h / 2
    };

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                if (mx >= start_button.x && mx < start_button.x + start_button.w &&
                    my >= start_button.y && my < start_button.y + start_button.h) {
                    for (int i = 0; i < NUM_REELS; ++i) {
                        reels[i].spinning = 1;
                        reels[i].stop_time = SDL_GetTicks() + 500 + i * 500;
                        reels[i].next_switch = 0;
                    }
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        for (int i = 0; i < NUM_REELS; ++i) {
            if (reels[i].spinning) {
                if (now >= reels[i].stop_time) {
                    reels[i].spinning = 0;
                }
                if (now >= reels[i].next_switch) {
                    reels[i].current = rand() % NUM_SYMBOLS;
                    reels[i].next_switch = now + 100;
                }
            }
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, background, NULL, NULL);
        for (int i = 0; i < NUM_REELS; ++i) {
            SDL_RenderCopy(ren, symbols[reels[i].current], NULL, &reel_rects[i]);
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
