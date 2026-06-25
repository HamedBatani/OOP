#include <SDL3/SDL.h>

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("Simulation Test", 800, 600, 0);
    SDL_Delay(2000);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
<<<<<<< HEAD
}
=======
}
>>>>>>> 46b0a3123dea584746bb590793b04d39d57df73e
