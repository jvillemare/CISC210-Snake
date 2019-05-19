/* snake.c
 * James Villemarette
 * May 17, 2019
 *
 * IMPORTANT NOTE: The snake only moves when the joystick moves.
 *
 * Plays a snake game on a 8x8 SenseHat pixel array using the
 * joystick, and the snake changes color depending on the
 * magnetic direction.
 *
 * Color Meanings:
 * ===============
 * Purple	Snake Head
 * Red 		Snake Body Part
 * Green 	Food pellet
 *
 * Whole 8x8 is red 	You failed! You either ate yourself or hit the
 *						the wall. The game will restart in 5 seconds.
 * Whole 8x8 is green 	You win! You got a snake to a full length of 63.
 */

#define _GNU_SOURCE
#define INCLUDE_HEAD 0
#define EXCLUDE_HEAD 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <linux/input.h>
#include <unistd.h>
#include <sense/sense.h>

int okayToRun = 1;

// create snake and food pellets
int snake_x[64];
int snake_y[64];

int food_pellet_x;
int food_pellet_y;

// manual import of font.c
uint16_t font[10][3][8]={
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
		{0xFFFF,0,0,0,0,0,0,0xFFFF},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//1
	{{0,0,0,0,0,0,0,0},{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},{0,0,0,0,0,0,0,0}},
	//2
	{{0xFFFF,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
		{0xFFFF,0,0,0,0xFFFF,0,0,0xFFFF},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0xFFFF}},
	//3
	{{0xFFFF,0,0,0xFFFF,0,0,0,0xFFFF},
		{0xFFFF,0,0,0xFFFF,0,0,0,0xFFFF},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//4
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0},
		{0,0,0,0xFFFF,0,0,0,0},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//5
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0xFFFF},
		{0xFFFF,0,0,0xFFFF,0,0,0,0xFFFF},
		{0xFFFF,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//6
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
		{0xFFFF,0,0,0,0xFFFF,0,0,0xFFFF},
		{0xFFFF,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//7
	{{0xFFFF,0,0,0,0,0,0,0},
		{0xFFFF,0,0,0,0,0,0,0},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//8
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
		{0xFFFF,0,0,0xFFFF,0,0,0,0xFFFF},
		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
	//9
	{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0xFFFF},
    	{0xFFFF,0,0,0xFFFF,0,0,0,0xFFFF},
    	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}},
};

/*
 * Function:  randomNumber
 * ---------------------------------
 * A random nubmer in [min, max].
 *
 * returns: a pseudo-random number in the range specified.
 */
int randomNumber(int min, int max) {

	int result = 0, low_num = 0, hi_num = 0;

	if (min < max) {
		low_num = min;
		hi_num = max + 1; // include max in output
	} else {
		low_num = max + 1; // include max in output
		hi_num = min;
	}

	result = (rand() % (hi_num - low_num)) + low_num;
	return result;

}

/*
 * Function:  initializeSnake
 * ---------------------------------
 * Starts a snake at 0 and puts a food pellet at 5, 5.
 *
 * returns: nothing.
 */
void initializeSnake() {

	memset(snake_x, -1, sizeof snake_x);
	memset(snake_y, -1, sizeof snake_y);

	snake_x[0] = 0;
	snake_y[0] = 0;

	food_pellet_x = 5;
	food_pellet_y = 5;

}

/*
 * Function:  snakeSize
 * ---------------------------------
 * Returns the number of elements in a snake.
 *
 * returns: how long the snake is.
 */
int snakeSize() {

	int size = 0;

	for(int i = 0; i < (int) sizeof(snake_x) / sizeof(snake_x[0]); i++) {
		if(snake_x[i] == -1)
			break;

		size = size + 1;
	}

	return size;

}

/*
 * Function:  moveSnake
 * ---------------------------------
 * Moves the snake by the specified amount.
 *
 * x: Amount to move head of snake, horizontally.
 * y: Amount to move head of snake, vertically.
 *
 * returns: nothing.
 */
void moveSnake(int x, int y) {

	int next_x = snake_x[0] + x;
	int next_y = snake_y[0] + y;

	int previous_x = snake_x[0];
	int previous_y = snake_y[0];

	snake_x[0] = next_x;
	snake_y[0] = next_y;

	for(int i = 1; i < snakeSize(); i++) {

		if(snake_x[i] == -1 || snake_y[i] == -1)
			break;

		next_x = snake_x[i];
		next_y = snake_y[i];

		snake_x[i] = previous_x;
		snake_y[i] = previous_y;

		previous_x = next_x;
		previous_y = next_y;

	}

}

/*
 * Function:  joystickHandler
 * ---------------------------------
 * Handles joystick inputs.
 *
 * returns: nothing.
 */
void joystickHandler(unsigned int code) {

	switch(code) {

		case KEY_UP:
			moveSnake(-1, 0);
			break;

		case KEY_RIGHT:
			moveSnake(0, 1);
			break;

		case KEY_LEFT:
			moveSnake(0, -1);
			break;

		case KEY_DOWN:
			moveSnake(1, 0);
			break;

		default:
			printf("Problem with receiving joystick input. Gracefully exiting...\n");
			okayToRun = 0;

	}

}

/*
 * Function:  drawSnake
 * ---------------------------------
 * Paints snake and food on FrameBuffer device.
 *
 * returns: nothing.
 */
void drawSnake(pi_framebuffer_t *fb, int snake_red, int snake_green, int snake_blue) {

	for(int i = 0; i < snakeSize(); i++) {
		if(snake_x[i] == -1 || snake_y[i] == -1)
			break;

		fb->bitmap->pixel[snake_x[i]][snake_y[i]] = getColor(snake_red, snake_green, snake_blue);
	}

}

/*
 * Function:  drawFoodPellet
 * ---------------------------------
 * Paints snake and food on FrameBuffer device.
 *
 * returns: nothing.
 */
void drawFoodPellet(pi_framebuffer_t *fb) {

	fb->bitmap->pixel[food_pellet_x][food_pellet_y] = getColor(0, 255, 0);

}

/*
 * Function:  draw
 * ---------------------------------
 * Paints snake and food on FrameBuffer device.
 *
 * returns: nothing.
 */
void draw(pi_framebuffer_t *fb, int snake_red, int snake_green, int snake_blue) {

	clearBitmap(fb->bitmap, 0);
	drawSnake(fb, snake_red, snake_green, snake_blue);
	drawFoodPellet(fb);

}

/*setFramebufferDigit
	screen: the bitmap to draw on
	digit: the single digit 0-9 to draw
	pos; the position in the matrix (0=left,1=right) to draw
	color: the color to draw the digit with
	draws a digit on the bitmap*/
void setMyFramebufferDigit(pi_framebuffer_t *screen, int digit, int pos, 
	uint16_t color) {

	int i, j, startRow, index;
	startRow = pos == 0 ? 0 : 5;

	for (i=0;i<8;i++){
		for (j=startRow, index=2; j < startRow + 3; j++, index--){
			screen->bitmap->pixel[i][j] = color & font[digit][j-startRow][i];
		}
	}

}

/*
 * Function:  spaceFree
 * ---------------------------------
 * See if part of the snake is occupying any space in the 8x8.
 *
 * returns: true if there's no snake at x and y, false if not.
 */
bool spaceFree(int x, int y, int modifier) {

	for(int i = 0 + modifier; i < snakeSize(); i++) {
		if(snake_x[i] == -1 || snake_y[i] == -1)
			break;

		if(snake_x[i] == x && snake_y[i] == y) {
			return false;
		}
	}

	return true;

}

/*
 * Function:  success
 * ---------------------------------
 * If the player enters a defined win state, then flash the screen
 * green for five seconds and print out stats.
 *
 * returns: nothing.
 */
void success(pi_framebuffer_t *fb) {

	printf("\nYou won!\nYour snake got to the max length of 63!\n");

	clearBitmap(fb->bitmap, 0);

	for(int i = 0; i < 5; i++) {

		clearBitmap(fb->bitmap, getColor(0, 255, 0));
		sleep(1);

	}

	clearBitmap(fb->bitmap, 0);

}

/*
 * Function:  failure
 * ---------------------------------
 * If the player enters a defined failure state, then flash the screen
 * red for five seconds, and print out stats.
 *
 * returns: nothing.
 */
void failure(pi_framebuffer_t *fb) {

	int sizeOfSnake = snakeSize();

	printf("\nYou failed!\nYour snake got to a length of %d\n", 
		sizeOfSnake);

	clearBitmap(fb->bitmap, 0);

	for(int i = 0; i < 5; i++) {

		clearBitmap(fb->bitmap, getColor(255, 0, 0));
		sleep(1);

		if(sizeOfSnake < 10) {
			setMyFramebufferDigit(fb, sizeOfSnake, 1, getColor(255, 255, 255));
			sleep(1);
		} else {
			setMyFramebufferDigit(fb, sizeOfSnake / 10, 0, getColor(255, 255, 255));
			sleep(1);
			setMyFramebufferDigit(fb, sizeOfSnake % 10, 1, getColor(255, 255, 255));
			sleep(1);
		}

	}

	clearBitmap(fb->bitmap, 0);

}

/*
 * Function:  handler
 * ---------------------------------
 * Handles control-c events.
 *
 * returns: nothing.
 */
void exitHandler() {

	okayToRun = 0;

	printf("\nGracefully exiting\n");

}

int main(int argc, char **argv) {

	signal(SIGINT, exitHandler);

	// setup for screen
	pi_framebuffer_t* fb = getFBDevice();

	clearBitmap(fb->bitmap, 0);

	// setup for joystick
	pi_joystick_t* joystick = getJoystickDevice();

	// setup for humidity sensor
	pi_i2c_t* device = geti2cDevice();
	coordinate_t data;

	int snake_red = 255; 
	int snake_green = 255; 
	int snake_blue = 255;

	if (device) {
		configureMag(device);
		getMagData(device, &data);
		
		snake_red = data.x;
		snake_green = data.y;
		snake_blue = data.z;
	}

	// setup randomness
	time_t t;
	srand((unsigned) time(&t));

	initializeSnake();

	while(okayToRun) {

		// read sensehat joystock and magnetic input
		if(device) {
			getMagData(device, &data);
			
			snake_red = data.x;
			snake_green = data.y;
			snake_blue = data.z;
		}

		pollJoystick(joystick, joystickHandler, 1000);

		// if head of snake is outside 8x8 wall or hits itself,
		// make the screen solid red for failure state
		if(snake_x[0] > 7 || snake_x[0] < 0 ||
			snake_y[0] > 7 || snake_y[0] < 0 || 
			!spaceFree(snake_x[0], snake_y[0], EXCLUDE_HEAD)) {

			// after a few seconds, restart snake
			failure(fb);
			initializeSnake();
			continue;

		}

		int SSize = snakeSize();

		// if snake head touching food pellet, replace the food pellet
		if(snake_x[0] == food_pellet_x && snake_y[0] == food_pellet_y) {

			// extend snake
			if(snake_x[SSize - 1] - 1 > 0 && spaceFree(snake_x[SSize - 1] - 1, snake_y[SSize - 1], INCLUDE_HEAD)) {
				// add to the left, if possible.
				snake_x[SSize] = snake_x[SSize - 1] - 1;
				snake_y[SSize] = snake_y[SSize - 1];
			} else if(snake_y[SSize - 1] - 1 > 0 && spaceFree(snake_x[SSize - 1], snake_y[SSize - 1] - 1, INCLUDE_HEAD)) {
				// add to the bottom, if possible.
				snake_x[SSize] = snake_x[SSize - 1];
				snake_y[SSize] = snake_y[SSize - 1] - 1;
			} else if(snake_x[SSize - 1] + 1 < 7 && spaceFree(snake_x[SSize - 1] + 1, snake_y[SSize - 1], INCLUDE_HEAD)) {
				// add to the right, if possible.
				snake_x[SSize] = snake_x[SSize - 1] + 1;
				snake_y[SSize] = snake_y[SSize - 1];
			} else if(snake_y[SSize - 1] + 1 < 7 && spaceFree(snake_x[SSize - 1], snake_y[SSize - 1] + 1, INCLUDE_HEAD)) {
				// add to the top, if possible.
				snake_x[SSize] = snake_x[SSize - 1];
				snake_y[SSize] = snake_y[SSize - 1] + 1;
			}

			// add a new food pellet not on snake
			bool lookingForNewPelletSpot = true;

			while(lookingForNewPelletSpot) {

				food_pellet_x = randomNumber(0, 7);
				food_pellet_y = randomNumber(0, 7);

				if(spaceFree(food_pellet_x, food_pellet_y, INCLUDE_HEAD))
					break;

			}

		}

		// if snake is at length, win
		if(SSize >= 63) {

			success(fb);
			exitHandler();

		}

		draw(fb, snake_red, snake_green, snake_blue);

	}

	// gracefully exiting

	clearBitmap(fb->bitmap, 0);

	freeFrameBuffer(fb);
	freeJoystick(joystick);

	if(device) {
		freei2cDevice(device);
	}

	return 0;

}

