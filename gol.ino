/**@file gol.ino  @brief main file for game of life */
#include "pattern_definitions.h"
#include "gol.h"

#include <MatrixHardware_Teensy3_ShieldV1toV3.h>    // SmartMatrix Shield for Teensy 3 V1-V3
#include <SmartMatrix.h>

const int defaultBrightness = 100; // range 0-255
                                                                
/* SmartMatrix configuration and memory allocation */
const uint16_t kMatrixWidth = 32;       // Set to the width of your display, must be a multiple of 8
const uint16_t kMatrixHeight = 32;      // Set to the height of your display
const uint8_t kRefreshDepth = 36;       // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_32ROW_MOD16SCAN;  // Choose the configuration that matches your panels.  See more details in MatrixCommonHub75.h and the docs: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);        // see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

//color definitions used for displaying cells on the matrix
const rgb24 COLOR_BLACK = {0, 0, 0 };
const rgb24 COLOR_RED {255, 0, 0};
const rgb24 COLOR_BLUE {0, 0, 255};
const rgb24 COLOR_CYAN {0, 200, 255};
const rgb24 COLOR_GREEN {0, 255, 0};
const rgb24 COLOR_ACID_GREEN {174, 255, 0};

//colors corresponding to the age of a cell
const rgb24 GEN1 = {225, 0, 255};
const rgb24 GEN2 = {13, 0, 255};
const rgb24 GEN3 = {0, 229, 255};
const rgb24 GEN4 = {0, 255, 30};
const rgb24 GEN5 = {255, 10, 10};

const rgb24 genColors[5] = {GEN1, GEN2, GEN3, GEN4, GEN5};

static uint16_t genCount = 0; //current generation of the board
static uint16_t aliveCellCount = 0; //variable to keep track of totall number of living cells. Allows program to reset board if it dies
cell_t boardCopies[NUM_BOARD_COPIES][32][32] = {0}; //store past boards for a comparision
uint8_t copyIndex  = 0; //current position in the board copies array

cell_t board[32][32] = {0}; //main board
cell_t tempBoard[32][32] = {0}; //holding board for calculating next generation

/**
 * arduino setup function. Runs once on startup, defines matrix parameters and initiates matrix object. Populates board depending on defined
 * value in gol.h
 */
void setup() {
    Serial.begin(115200);
    matrix.addLayer(&backgroundLayer); // Initialize matrix
    matrix.setBrightness(defaultBrightness);
    matrix.begin();
    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();
    //matrix.setRotation(90);

    #if defined(PURE_GLIDER)
      createIdealGlider();
    #elif defined(RANDOM_BOARD)
      Serial.printf("value of analog read:   %.4f\n", analogRead(0));
      randomSeed(analogRead(14));
      populateBoard();
     #endif
    delay(1000);
}

/**
 * Arduino main function. This will evolve the board and update the screen depending on the GEN_DELAYI variable value
 */
void loop() {
  if(evolve())
  {
    //the board changed between generations
    genCount++;
    if(aliveCellCount == 0)
    {
      Serial.println("all cells are dead");
      resetBoard("Dead", COLOR_RED);
    }
    #ifdef CLEAR_ON_LOOP
    if(copyIndex < NUM_BOARD_COPIES)
    {
      storeBoardCopy();
    }
    else
    {
      //with NUM_BOARD_COPIES boards stored, can check for loop case now
      if(boardLooping())
      {
        resetBoard("Looping", COLOR_ACID_GREEN);
      }
      else
      {
        copyIndex = 0; //reset copy index
        memset(boardCopies, 0, sizeof(boardCopies)); //clear old copy data
      }
    }
    #endif
  }
  else
  {
    //board is stuck in current state, reset it
    Serial.println("board is frozen, reset it");
    resetBoard("Frozen", COLOR_CYAN);
    
  }
  #ifdef DEBUG_MODE
    printBoardSerial(); 
  #endif
  delay(GEN_DELAY);
}

/**
 * Function to iterate through the boardCopies array to check for any repeating boards, indicating there is a loop.
 * This will not be called if CLEAR_ON_LOOP is not defined 
 */
bool boardLooping()
{
    bool repeatingBoard = true; //true if compared boards are equal
    //k starts at 2 because we can assume that boards 0 and 1 are not identical, that would be caught by the evolve function check
    for(int k = 2; k < NUM_BOARD_COPIES; k++)
    {
      for(int i = 0; i < matrix.getScreenWidth(); i++)
      {
        for(int j = 0; j < matrix.getScreenHeight(); j++)
        {
          if(boardCopies[0][i][j].status != boardCopies[k][i][j].status)
          {
            repeatingBoard = false;
          }
        }
      }
      if(repeatingBoard == true)
      {
        return true;
      }
      repeatingBoard = true; //reset for next comparision
    }
    return false;
}

/**
 *  Function to clear the board. It will print the specified message with the specified color for 5 seconds before updating the board
 *  with the desired reset pattern select from #define variables
 *  
 *  @param message String with the message to be displayed. Should be <10 characters 
 *  @param color Rgb24 color used to set the color of message
 */
void resetBoard(const char* message, rgb24 color)
{
      clearBoard(); //empty in case we are resetting a frozen board
      Serial.println("board is cleared");
      delay(1000);
      backgroundLayer.fillScreen(COLOR_BLACK);
      backgroundLayer.drawString(6, matrix.getScreenHeight() / 2, color, message);
      backgroundLayer.swapBuffers(false);
      delay(5000);
      aliveCellCount = 0; //set alive cell count to zero before setting cells alive
      
      #if defined(PURE_GLIDER)
        createIdealGlider();
      #elif defined(RANDOM_BOARD)
        populateBoard();
       #endif
       
      Serial.println("board is reset");
      delay(1000);
      genCount = 0; //starting back at generation 0
      copyIndex = 0; //reset copy index
      memset(boardCopies, 0, sizeof(boardCopies)); //clear old copy data
}

/**
 * This will create an ideal glider pattern in the middle of the board. After 25ish generations, 4 gliders will be generated
 */
void createIdealGlider()
{
  uint8_t pgX = 0;
  uint8_t pgY = 0; //variables to map i and j to pure glider array, as i and j don't start at 0
  backgroundLayer.fillScreen(COLOR_BLACK); //ensure there are no artifacts from the previous screen
  uint8_t xStart = (matrix.getScreenWidth()/2)-(IDEAL_GLIDER_ROWS/2); //want to find the middle point, then subtract half of the width of the pure glider matrix to determine mid point
  uint8_t yStart =  (matrix.getScreenWidth()/2)-(IDEAL_GLIDER_COLS/2); //same equation described above
  #ifdef DEBUG_MODE
    Serial.printf("Ideal glider (x, y) start pos is (%d, %d)\n", xStart, yStart);
  #endif
  for(int i = xStart; i < xStart + IDEAL_GLIDER_ROWS;  i++)
  {
    for(int j = yStart;  j < yStart + IDEAL_GLIDER_COLS; j++)
    {
      pgX = i - xStart;
      pgY = j - yStart;
      board[i][j].status = IDEAL_GLIDER_ARRAY[pgX][pgY];
      if(board[i][j].status == ALIVE)
      {
        aliveCellCount += 1;
        backgroundLayer.drawPixel(i, j, GEN1);
      }
      else
      {
        backgroundLayer.drawPixel(i, j, COLOR_BLACK); //just in case
      }
    }
  }
  backgroundLayer.swapBuffers(false);
}

/**
 * Function to print the current board to the serial console. Prints 2 new lines between each board
 */
void printBoardSerial()
{
  Serial.printf("Generation %d\n", genCount);
    for(int i = 0; i < matrix.getScreenWidth(); i++)
    {
      for(int j = 0; j < matrix.getScreenHeight(); j++)
      {
      if(board[i][j].status == 1)
      {
        Serial.print("*");
      }
      else
      {
        Serial.print("-");
      }
    }
    Serial.println("");
  }
  Serial.println("");
  Serial.println("");
  
}

/**
 * Function that determines the state of the next generation board. Will check if the next gen and previous boards are identical and return false if this occurs
 */
bool evolve()
{
  uint8_t count = 0;
  uint8_t curStatus = 0;
  memcpy(tempBoard, board, sizeof(board)); //copy existing board to copy to modify
    for(int i = 0; i < matrix.getScreenWidth(); i++)
    {
      for(int j = 0; j < matrix.getScreenHeight(); j++)
      {
      curStatus = board[i][j].status; //get current status of previous board
      count = countNeighbors(i, j);     
      if((curStatus == ALIVE) && (count == 2 || count == 3))
      {
        //cell lives on another generation,,update color as needed
        if(tempBoard[i][j].age < 5)
        {
          backgroundLayer.drawPixel(i, j, genColors[tempBoard[i][j].age]);
          tempBoard[i][j].age++; //cell lives on another generation, increment the age counter to update the color output
        }
        else
        {
          backgroundLayer.drawPixel(i, j, GEN5);
        }
      }
      /*
      else if(curStatus == ALIVE && (count <= 1 || count >=4))
      {
        tempBoard[i][j].status = DEAD; //died from overpopulation or underpopulation
        aliveCellCount--;
        tempBoard[i][j].age = 0;
        backgroundLayer.drawPixel(i, j, COLOR_BLACK);
      }
      */
      else if(curStatus == DEAD && count == 3)
      {
        tempBoard[i][j].status = ALIVE;
        aliveCellCount++;
        backgroundLayer.drawPixel(i, j, GEN1);
      }
      else
      {
        tempBoard[i][j].status = DEAD;
        tempBoard[i][j].age = 0;
        backgroundLayer.drawPixel(i, j, COLOR_BLACK); //still need to fill copy buffer with black to ensure there are no carryovers
      }
    }
  }
  if(compareBoards())
  {
    //boards are identical after evolution, therefore we should reset the board.
    backgroundLayer.swapBuffers(false); //update the matrix, ignore return value
    return false;
  }
  memcpy(board, tempBoard, sizeof(board)); //save modified board to main board
  backgroundLayer.swapBuffers(false); //update the matrix, ignore return value
  return true;
}

/**
 * Function to reset the board array and clear the matrix display
 */
void clearBoard()
{
  for(int i = 0; i < matrix.getScreenWidth(); i++)
  {
    for(int j = 0; j < matrix.getScreenHeight(); j++)
    {
      board[i][j].status = DEAD;
      board[i][j].age = 0;
    }
  }
  backgroundLayer.fillScreen(COLOR_BLACK);
  backgroundLayer.swapBuffers(false);
}

/**
 * Function to randomly populate the board on start/restart. 
 * To set the likelyhood of a cell being alive, set the LIKELYHOOD variable in gol.h. 100-LIKELYHOOD = % chance of being alive
 */
void populateBoard()
{
  for(int i = 0; i < matrix.getScreenWidth(); i++)
  {
    for(int j = 0; j < matrix.getScreenHeight(); j++)
    {
      board[i][j].status = random(0,100)  > (100-LIKELYHOOD) ? 1 : 0;
      aliveCellCount += board[i][j].status;
      backgroundLayer.drawPixel(i, j, board[i][j].status == 1 ? GEN1 : COLOR_BLACK);
    }
  }
  backgroundLayer.swapBuffers(false);
}

/**
 * Copy the current board to the current index of boardCopies for later comparision
 */
void storeBoardCopy()
{
    for(int i = 0; i < matrix.getScreenWidth(); i++)
    {
      for(int j = 0; j < matrix.getScreenHeight(); j++)
      {
        boardCopies[copyIndex][i][j] = board[i][j];
      }
    }
    copyIndex++; 
}

/*
 * Function to compare the previous gen board to the current generation. Returns true if they are identical
 */
bool compareBoards()
{
    for(int i = 0; i < kMatrixWidth; i++)
    {
      for(int j = 0; j < kMatrixHeight; j++)
      {
        if(board[i][j].status != tempBoard[i][j].status)
        {
          return false;
        }
      }
    }
    return true;
}

/**
 * Helper function used during evolution to count the number of living neighbors a cell has.
 * 
 * @param x the x position of the cell that is being checked
 * @params y the y position of the cell that is being checked
 */
uint8_t countNeighbors(uint8_t x, uint8_t y)
{
  uint8_t count = 0;
  for(int i = x-1; i<x+2; i++)
  {
    for(int j = y-1; j < y+2; j++)
    {
      if((i > -1 && j > -1) && (i < kMatrixHeight && j < kMatrixWidth))
      {
        if(!(i == x && j == y))
        {
          count += board[i][j].status;
        }
      }
    }
  }
  return count;
}
