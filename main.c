#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400
#define MAX_ITER 400
//#define WINDOW_WIDTH 320
//#define WINDOW_HEIGHT 240
//#define MAX_ITER 2000
#define ZOOM_FACTOR 0.9
#define CURSOR_MOVE_STEP 5
#define MIN_ZOOM 1e-16

typedef struct {
    double x, y;
    double zoom;
} Fractal;

typedef struct {
    int x, y;
    Fractal fractal;
} ZoomState;

ZoomState zoomStack[10];
int zoomStackIndex = 0;

void handleControllerInput(SDL_Event *e, int *cursorX, int *cursorY, Fractal *fractal, int *moveRight, int *moveLeft, int *moveUp, int *moveDown) {
    if (e->type == SDL_CONTROLLERBUTTONDOWN) {
        switch (e->cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                *moveUp = 1;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                *moveDown = 1;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                *moveLeft = 1;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                *moveRight = 1;
                break;
            case SDL_CONTROLLER_BUTTON_A:
                if (zoomStackIndex < 10) {
                    zoomStack[zoomStackIndex].x = *cursorX;
                    zoomStack[zoomStackIndex].y = *cursorY;
                    zoomStack[zoomStackIndex].fractal = *fractal;
                    zoomStackIndex++;
                }
                fractal->x += (*cursorX - WINDOW_WIDTH / 2) * fractal->zoom;
                fractal->y += (*cursorY - WINDOW_HEIGHT / 2) * fractal->zoom;
                fractal->zoom *= ZOOM_FACTOR;
                break;
            case SDL_CONTROLLER_BUTTON_B:
                if (zoomStackIndex > 0) {
                    zoomStackIndex--;
                    *cursorX = zoomStack[zoomStackIndex].x;
                    *cursorY = zoomStack[zoomStackIndex].y;
                    *fractal = zoomStack[zoomStackIndex].fractal;
                }
                break;
        }
    } else if (e->type == SDL_CONTROLLERBUTTONUP) {
        switch (e->cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                *moveUp = 0;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                *moveDown = 0;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                *moveLeft = 0;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                *moveRight = 0;
                break;
        }
    }
}

int compute_color(double zx, double zy, double x0, double y0) {
    int iter;
    for (iter = 0; iter < MAX_ITER; ++iter) {
        if (zx * zx + zy * zy >= 4.0) break;
        double temp = zx * zx - zy * zy + x0;
        zy = 2.0 * zx * zy + y0;
        zx = temp;
    }
    return iter;
}

void render_fractal(SDL_Renderer* renderer, Fractal* fractal) {
    for (int py = 0; py < WINDOW_HEIGHT; ++py) {
        for (int px = 0; px < WINDOW_WIDTH; ++px) {
            double x0 = (px - WINDOW_WIDTH / 2) * fractal->zoom + fractal->x;
            double y0 = (py - WINDOW_HEIGHT / 2) * fractal->zoom + fractal->y;

            int iter = compute_color(0.0, 0.0, x0, y0);

            int r = (iter * 9) % 256;
            int g = (iter * 2) % 256;
            int b = (iter * 3) % 256;
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
}

void render_cursor(SDL_Renderer* renderer, int cursorX, int cursorY) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderDrawLine(renderer, cursorX - 10, cursorY, cursorX + 10, cursorY);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, cursorX, cursorY - 10, cursorX, cursorY + 10);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int w = 0; w < 4; w++) {
        for (int h = 0; h < 4; h++) {
            int dx = 2 - w;
            int dy = 2 - h;
            if ((dx * dx + dy * dy) <= (2 * 2)) {
                SDL_RenderDrawPoint(renderer, cursorX + dx, cursorY + dy);
            }
        }
    }
}

void reset_fractal(Fractal *fractal) {
    fractal->x = 0.0;
    fractal->y = 0.0;
    fractal->zoom = 4.0 / WINDOW_WIDTH;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Fractal",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GameController* controller = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) break;
        }
    }

    if (!controller) {
        printf("No game controller found.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Fractal fractal = {0.0, 0.0, 4.0 / WINDOW_WIDTH};
    int cursorX = WINDOW_WIDTH / 2;
    int cursorY = WINDOW_HEIGHT / 2;

    int moveRight = 0, moveLeft = 0, moveUp = 0, moveDown = 0;

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else {
                handleControllerInput(&event, &cursorX, &cursorY, &fractal, &moveRight, &moveLeft, &moveUp, &moveDown);
            }
        }

        if (moveRight) cursorX = (cursorX < WINDOW_WIDTH - CURSOR_MOVE_STEP) ? cursorX + CURSOR_MOVE_STEP : WINDOW_WIDTH - 1;
        if (moveLeft) cursorX = (cursorX > CURSOR_MOVE_STEP) ? cursorX - CURSOR_MOVE_STEP : 0;
        if (moveUp) cursorY = (cursorY > CURSOR_MOVE_STEP) ? cursorY - CURSOR_MOVE_STEP : 0;
        if (moveDown) cursorY = (cursorY < WINDOW_HEIGHT - CURSOR_MOVE_STEP) ? cursorY + CURSOR_MOVE_STEP : WINDOW_HEIGHT - 1;

        if (fractal.zoom < MIN_ZOOM) {
            reset_fractal(&fractal);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_fractal(renderer, &fractal);
        render_cursor(renderer, cursorX, cursorY);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
