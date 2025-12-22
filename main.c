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
        SDL_Log("SDL_CreateRenderer failed (%s), retrying with software renderer", SDL_GetError());
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    }
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

    int bg_w, bg_h;
    SDL_QueryTexture(background, NULL, NULL, &bg_w, &bg_h);
    float bg_scale = (float)win_w / bg_w;
    if ((float)win_h / bg_h < bg_scale) {
        bg_scale = (float)win_h / bg_h;
    }

    int bg_render_w = (int)(bg_w * bg_scale);
    int bg_render_h = (int)(bg_h * bg_scale);
    int bg_render_x = (win_w - bg_render_w) / 2;
    int bg_render_y = (win_h - bg_render_h) / 2;

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
     * Reel placement tuned from pixel measurements of FruitMachineBody.jpeg
     * (2048x2048).  Ratios keep the layout aligned as the background scales.
     */
    const float reel_centers[NUM_REELS] = {0.2580f, 0.4929f, 0.7310f};
    const float reel_y_center = 0.3950f;
    const float reel_window_height_ratio = 0.1030f;
    const float reel_symbol_width_ratio = 0.1670f;
    const float reveal_ratio = 0.30f; /* show 30% of symbol above/below */

    int reel_w = (int)(bg_render_w * reel_symbol_width_ratio);
    int reel_visible_h = (int)(bg_render_h * reel_window_height_ratio);
    int reel_h = (int)(reel_visible_h / (1.0f + 2.0f * reveal_ratio));
    int spin_speed = reel_h / 8;
    int reel_margin = (reel_visible_h - reel_h) / 2;

    SDL_Rect reel_rects[NUM_REELS];
    for (int i = 0; i < NUM_REELS; ++i) {
        reel_rects[i].w = reel_w;
        reel_rects[i].h = reel_visible_h;
        reel_rects[i].x = bg_render_x + (int)(bg_render_w * reel_centers[i] - reel_w / 2);
        reel_rects[i].y = bg_render_y + (int)(bg_render_h * reel_y_center - reel_h / 2) - reel_margin;
    }

    /* Start button bounding box from image analysis (centered near 0.5, 0.77). */
    SDL_Rect start_button = {
        bg_render_x + (int)(bg_render_w * 0.3500f),
        bg_render_y + (int)(bg_render_h * 0.7000f),
        (int)(bg_render_w * 0.3000f),
        (int)(bg_render_h * 0.1390f)
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
        SDL_Rect bg_rect = {bg_render_x, bg_render_y, bg_render_w, bg_render_h};
        SDL_RenderCopy(ren, background, NULL, &bg_rect);
        for (int i = 0; i < NUM_REELS; ++i) {
            SDL_RenderSetClipRect(ren, &reel_rects[i]);
            int base_y = reel_rects[i].y + reel_margin;
            SDL_Rect dest = {reel_rects[i].x,
                             base_y - reel_h + (int)reels[i].offset,
                             reel_w,
                             reel_h};
            SDL_RenderCopy(ren, symbols[reels[i].prev], NULL, &dest);
            dest.y = base_y + (int)reels[i].offset;
            SDL_RenderCopy(ren, symbols[reels[i].current], NULL, &dest);
            dest.y = base_y + reel_h + (int)reels[i].offset;
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
