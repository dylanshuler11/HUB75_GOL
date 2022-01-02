/**@file pattern_definitions.h  @brief contains definitions for predefined board starting patterns */

#ifndef PATTERN_DEFINITIONS_H
#define PATTERN_DEFINITIONS_H
#endif

#include <stdint.h>

const uint8_t IDEAL_GLIDER_ROWS = 9;
const uint8_t IDEAL_GLIDER_COLS = 15;
const uint8_t IDEAL_GLIDER_ARRAY[9][15] =     { {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
                                                                                        {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0}, 
                                                                                        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0}, 
                                                                                        {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}, 
                                                                                        {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0}, 
                                                                                        {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}, 
                                                                                        {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1},
                                                                                        {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0}, 
                                                                                        {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0} };
