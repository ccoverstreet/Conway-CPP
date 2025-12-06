/* primitives.c ... */

/*
 * This example creates an SDL window and renderer, and then draws some lines,
 * rectangles and points to it every frame.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <omp.h>
#include <unistd.h>
#include <math.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
#define ROWS 1500
#define COLS 1500
int GRID[ROWS][COLS];
int GRID_2[ROWS][COLS];
int grid_select = 1;
SDL_Texture *texture;
int camera_x = 0;
int camera_y = 0;
int camera_w = COLS;
int camera_h = ROWS;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    int i;


    SDL_SetAppMetadata("Example Renderer Primitives", "1.0", "com.example.renderer-primitives");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("CONWAY", COLS, ROWS, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, COLS, ROWS, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 1);

	srand(time(NULL));
	for (size_t i = 1; i < ROWS - 1; i++) {
		for (size_t j = 1; j < COLS - 1; j++) {
 			GRID[i][j] = 0;
 			if (rand() / (float)RAND_MAX < 0.5) {
 				GRID[i][j] = 1;
 			} else {
 				GRID[i][j] = 0;
 			}

		}
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, COLS, ROWS);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

int max(int a, int b) {
	return a > b ? a : b;
}

int min(int a, int b) {
	return a < b ? a : b;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
    	if (event->key.key == SDLK_W) {
    		if (camera_y >= 5) camera_y = camera_y - 5;
    	} else if (event->key.key == SDLK_S) {
    		if (camera_y < ROWS - camera_h) camera_y = camera_y + 5;
    	} else if (event->key.key == SDLK_A) {
			if (camera_x >= 5) camera_x = camera_x - 5;
    	} else if (event->key.key == SDLK_D) {
			if (camera_x < COLS - camera_w) camera_x = camera_x + 5;

    	} else if (event->key.key == SDLK_E)  {
    		int big_w = min(min(1.1 * camera_w, ROWS - camera_y), COLS);
    		int big_h = min(min(1.1 * camera_h, COLS - camera_x), ROWS);
    		camera_w = min(big_w, big_h);
    		camera_h = min(big_w, big_h);
    	} else if (event->key.key == SDLK_Q)  {
			camera_w = 0.9 * camera_w;
			camera_h = 0.9 * camera_h;
    	} else if (event->key.key == SDLK_R) {
    		camera_x = 0;
    		camera_y = 0;
    		camera_w = COLS;
    		camera_h = ROWS;
    	}
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

size_t frame_counter = 1;
size_t n_frames = 1;


/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
	//sleep(1);
	if (grid_select == 0) {
		grid_select = 1;
	} else {
		grid_select = 0;
	}

	int (*prev_grid)[COLS] = NULL;
	int (*grid)[COLS] = NULL;
	if (grid_select == 0) {
		grid = GRID;
		prev_grid = GRID_2;
	} else {
		grid = GRID_2;
		prev_grid = GRID;
	}


	// Update grid using rules

	double start_time = omp_get_wtime();
	for (size_t i = 1; i < ROWS - 1; i++) {
		for (size_t j = 1; j < COLS - 1; j++) {
			int neighbor_sum = prev_grid[i-1][j-1] + prev_grid[i-1][j] + prev_grid[i-1][j+1]
				+ prev_grid[i][j-1] + prev_grid[i][j+1]
				+ prev_grid[i+1][j-1] + prev_grid[i+1][j] + prev_grid[i+1][j+1];

			bool is_alive = prev_grid[i][j] == 1;

			if (is_alive) {
				if (neighbor_sum < 2) {
					grid[i][j] = 0;
				} else if (neighbor_sum < 4) {
					grid[i][j] = 1;
				} else {
					grid[i][j] = 0;
				}
			} else {
				if (neighbor_sum == 3) grid[i][j] = 1;
			}
		}
	}


	double afterupdate_time = omp_get_wtime();



	for (size_t i = 1; i < ROWS - 1; i++) {
		for(size_t j = 1; j < COLS - 1; j++) {
			prev_grid[i][j] = 0;
		}
	}

	double afterzero_time = omp_get_wtime();

	void *pixels;
	int pitch;
	SDL_LockTexture(texture, NULL, &pixels, &pitch);
	uint32_t *row = (uint32_t*)pixels;
	for (int j = 1; j < ROWS - 1; j++) {
		for (int i = 1; i < COLS - 1; i++) {
			if (grid[i][j] == 1) {
				row[i * COLS + j] = 0xffffffff;
			} else {
				row[i * COLS + j] = 0x0;
			}
		}
	}

	SDL_UnlockTexture(texture);

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

	//SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_FRect src = {
		camera_x, camera_y, 
		camera_w, camera_h
	};
	SDL_FRect dst = {0, 0, COLS * 1, ROWS * 1};
	SDL_RenderTexture(renderer, texture, &src, &dst);





	/*
	for (size_t i = 0; i < ROWS; i++) {
		for (size_t j = 0; j < COLS; j++) {
			if (grid[i][j] == 1) {
				SDL_RenderPoint(renderer, j, i);
			}
		}
	}
	*/

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

	double end_time = omp_get_wtime();

	printf("%f %f %f %f\n", start_time - start_time, afterupdate_time - start_time, afterzero_time - start_time, end_time - start_time);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}




