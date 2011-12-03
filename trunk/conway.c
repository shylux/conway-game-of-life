/*
 ============================================================================
 Name        : conway.c
 Author      : Shylux
 Version     : 0.1
 Copyright   : None
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include <pthread.h>

//bool
typedef unsigned char bool;
#define true   (1)
#define false  (0)

//preferences
#define SCREEN_WIDTH		1000
#define SCREEN_HEIGHT		1000
#define	SCREEN_BPP		32
#define	SCREEN_ZOOM		1
#define	DELAY			0
#define EXIT_TIME		-1
#define SHOW_EVOL_COUNTER	false
#define SHOW_FPS		true
int EVOL_COUNTER = 0;
const char* TITLE = "Conway's Spiel des Lebens";

//Calculation
bool field[SCREEN_WIDTH][SCREEN_HEIGHT];
bool laststate[SCREEN_WIDTH][SCREEN_HEIGHT];
pthread_t thread_calc;


//SDL
SDL_Surface *screen = NULL;
SDL_Surface *message_fps = NULL;
Uint32 PIXEL_BLACK;
Uint32 PIXEL_WHITE;
SDL_Event event;
bool QUIT_STATE = false;
pthread_t thread_draw;

/*
 * All inital stuff like SDL_Init, setting variables and setting up the screen.
 */
bool init() {
	//initialize sdl
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		printf("can't initialize sdl\n");
		return false;
	}
	screen = SDL_SetVideoMode(SCREEN_WIDTH*SCREEN_ZOOM, SCREEN_HEIGHT*SCREEN_ZOOM, SCREEN_BPP, SDL_SWSURFACE);
	//check if screen sucess
	if (screen == NULL) {
		printf("can't set up screen\n");
		return false;
	}

	//initialize pixels
	PIXEL_BLACK = SDL_MapRGB(screen->format, 0, 0, 0);
	PIXEL_WHITE = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);

	//set title
	SDL_WM_SetCaption(TITLE, NULL);

	return true;
}

/*
 * Called at the end of the Program. Used to free the memory.
 */
void clean_up() {
	//quit sdl
	SDL_Quit();
}

/*
 * Draws a pixel to the given position.
 */
void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel ) {
	//Convert the pixels to 32 bit
	Uint32 *pixels = (Uint32 *)surface->pixels;
	//Set the pixel
	pixels[ ( y * surface->w ) + x ] = pixel;
}

//check boolean value at position (x,y). Emulates a torus field.
bool get_pos(int x, int y) {
	//torus: if you left at bottom you enter at the top; same horizontally.
	if (x < 0) x = SCREEN_WIDTH - 1;
	if (x == SCREEN_WIDTH) x = 0;
	if (y < 0) y = SCREEN_HEIGHT - 1;
	if (y == SCREEN_HEIGHT) y = 0;
	return field[x][y];
}

/*
 * Draws the actual state to the screen.
 */
void *update_screen_all() {
	int x, y, zx, zy;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			for (zx = 0; zx < SCREEN_ZOOM; zx++) {
				for (zy = 0; zy < SCREEN_ZOOM; zy++) {
					if (field[x][y]) {
						put_pixel32(screen, x * SCREEN_ZOOM + zx, y * SCREEN_ZOOM + zy, PIXEL_BLACK);
					} else {
						put_pixel32(screen, x * SCREEN_ZOOM + zx, y * SCREEN_ZOOM + zy, PIXEL_WHITE);
					}
				}
			}
		}
	}
	return NULL;
}

/*
 * Sets all pixel in the window to white.
 * Only have effect on the screen an not on the calculation.
 */
void clear_screen() {
	int x, y;
	for (x = 0; x < SCREEN_WIDTH*SCREEN_ZOOM; x++) {
		for (y = 0; y < SCREEN_HEIGHT*SCREEN_ZOOM; y++) {
			put_pixel32(screen, x, y, PIXEL_WHITE);
		}
	}
}

/*
 * Deprecated:
 * Counting black pixels around the given position
 */
int count_neighbours_old(int testx, int testy) {
	int count = 0, x, y;

	for (x = testx -1; x < testx + 2; x++) {
		for (y = testy - 1; y < testy + 2; y++) {
			//don't count the position itself
			if (x == testx && y == testy) continue;
			if (get_pos(x,y)) count++;
		}
	}
	return count;
}

/*
 * Counting black pixels around the position (8 neighbours)
 * Faster because i don't use a for-loop and just use the ger_pos() if the pixel is at the border of the window.
 */
int count_neighbours(int x, int y) {
	int count = 0;

	if (x == 0 || y == 0 || x == SCREEN_WIDTH -1 || y == SCREEN_HEIGHT - 1) {
		if (get_pos(x-1,y-1)) count++;
		if (get_pos(x-1,y)) count++;
		if (get_pos(x-1,y+1)) count++;
		if (get_pos(x,y-1)) count++;
		if (get_pos(x,y+1)) count++;
		if (get_pos(x+1,y-1)) count++;
		if (get_pos(x+1,y)) count++;
		if (get_pos(x+1,y+1)) count++;
	} else {
		if (field[x-1][y-1]) count++;
		if (field[x-1][y]) count++;
		if (field[x-1][y+1]) count++;
		if (field[x][y-1]) count++;
		if (field[x][y+1]) count++;
		if (field[x+1][y-1]) count++;
		if (field[x+1][y]) count++;
		if (field[x+1][y+1]) count++;
	}

	return count;
}

/*
 * Calculates the next State
 */
void *update_field_nextgen() {
	//copy the field to manipulate
	bool newfield[SCREEN_WIDTH][SCREEN_HEIGHT];
	memcpy(newfield, field, SCREEN_WIDTH*SCREEN_HEIGHT);
	memcpy(laststate, field, SCREEN_WIDTH*SCREEN_HEIGHT);
	int x, y;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			switch (count_neighbours(x, y)) {
			case 0:
			case 1:
				newfield[x][y] = false;
				break;
			case 2:
				break;
			case 3:
				newfield[x][y] = true;
				break;
			default:
				newfield[x][y] = false;
				break;
			}
		}
	}
	memcpy(field, newfield, SCREEN_WIDTH*SCREEN_HEIGHT);
	return NULL;
}

/*
 * Sets the pixels randomly. You can select the percent of black pixel (%)
 */
void load_startconf_random(double percent_black) {
	//initialize randomizer
	srand(time(NULL));
	int random, x, y;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			random = rand()%100;
			if (random < percent_black) {
				field[x][y] = true;
			} else {
				field[x][y] = false;
			}
		}
	}
}
/*
 * Draws a given figure in the middle of the window.
 */
void load_startconf_example() {
	//set all pixel to white
	int x, y;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			field[x][y] = false;
		}
	}
	//insert given figure
	x = SCREEN_WIDTH / 2;
	y = SCREEN_HEIGHT / 2;
	field[x+1][y] = true;
	field[x+2][y] = true;
	field[x][y+1] = true;
	field[x+1][y+1] = true;
	field[x+1][y+2] = true;
}
/*
 * Sets all pixels to black
 */
void load_startconf_allblack() {
	int x, y;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			field[x][y] = true;
		}
	}
}
/*
 * Sets all pixels to white
 */
void load_startconf_allwhite() {
	int x, y;
	for (x = 0; x < SCREEN_WIDTH; x++) {
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			field[x][y] = false;
		}
	}
}

time_t lastfpsupdatetime;
int lastfpsupdatevalue;
/*
 * Calculates Frames per Second
 * Output in Window Title or in Console
 */
void fps() {
	time_t ac;
	ac = time(NULL);
	if (ac > lastfpsupdatetime) {
		//printf("%d fps ########\n", EVOL_COUNTER - lastfpsupdatevalue);
		char temp[100];
		sprintf(temp,"%s - %d fps", TITLE, EVOL_COUNTER - lastfpsupdatevalue);
		SDL_WM_SetCaption(temp , NULL);

		lastfpsupdatetime = ac;
		lastfpsupdatevalue = EVOL_COUNTER;
	}
}

// main
int main(void) {
	printf("Hy\n");

	//Start SDL
	if (!init()) return EXIT_FAILURE;

	//clear screen
	clear_screen();

	//startconfig
	load_startconf_random(10);
	memcpy(laststate, field, SCREEN_WIDTH*SCREEN_HEIGHT);
	update_screen_all();
	if (SDL_Flip(screen) == -1) return EXIT_FAILURE;
	//SDL_Delay(0);

	//set the init time
	lastfpsupdatetime = time(NULL);
	lastfpsupdatevalue = 0;

	while (!QUIT_STATE) {
		//increase evol_counter
		EVOL_COUNTER++;

		//check for quit event
		while( SDL_PollEvent( &event )) {
			if (event.type == SDL_QUIT) QUIT_STATE = true;
		}

		pthread_create(&thread_draw, NULL, update_screen_all, &EVOL_COUNTER);
		pthread_create(&thread_calc, NULL, update_field_nextgen, &EVOL_COUNTER);

		pthread_join (thread_calc, NULL);
		pthread_join (thread_draw, NULL);

		//update_screen_all();
		if (SDL_Flip(screen) == -1) return EXIT_FAILURE;

		//Counter on command line
		if (SHOW_EVOL_COUNTER) {
			printf("EVOL_COUNTER %d\n", EVOL_COUNTER);
			fflush(stdout);
		}

		//fps
		if (SHOW_FPS) fps();

		if (DELAY > 0) SDL_Delay(DELAY);
	}
	clean_up();
	printf("Bye\n");
	return EXIT_SUCCESS;
}
