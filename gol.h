/**@file gol.h  @brief header file containing structs and predefined variables for game of life */
#ifndef GOL_H
#define GOL_H
#endif

#define SD_CS BUILTIN_SDCARD
#define ALIVE 1
#define DEAD 0

//#define PURE_GLIDER //if uncommented, a pure glider pattern will be produced in the middle of the screen on start/reset

//comment this out to select a pattern to reset to 
#define RANDOM_BOARD
#define LIKELYHOOD 55

#define CLEAR_ON_LOOP //define this to allow the board to reset itself if it gets stuck in a bigenerational cycle

//#define DEBUG_MODE //uncomment this to allow for extra serial messages and printing the board to console.

#define COLOR_DEPTH 24    // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
#define NUM_BOARD_COPIES 4
#define GEN_DELAY 250 //delay between board evolutions, in milliseconds

struct cell{
  uint8_t status = DEAD;
  uint8_t age = 0;
}typedef cell_t;
