#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd/lcd.h"
#include "svgrgb565.h"
#include "switches.h"

#define screenWidth 240
#define screenHeight 320

#define trackWidth 22
#define numTracks 14
#define goalTrack 1
#define numOfGoals 5
#define goalBGColour DARK_GREEN
#define unitLength 20
#define offsetPx 12
#define roadOffset 3

#define numRoadLanes 11
#define maxVehiclesPerLane 3

#define frogSize 12
#define frogColour LIME_GREEN
#define frogStepX 22

#define scoreMovement 10;

void drawVehicles();
void drawFrog();
void drawRoads();
void drawGoals();
void updateVehicles();
void collision();
void resetFrog();
void deathHandler();
void drawStats();
void putText(uint8_t t, uint16_t x, uint16_t y);

const uint16_t colours[numTracks] = {BLACK,FOREST_GREEN,DARK_BLUE,DARK_BLUE,DARK_BLUE,DARK_BLUE,DARK_BLUE,DARK_VIOLET,BLACK,BLACK,BLACK,BLACK,BLACK,DARK_VIOLET};
const uint8_t goalPositions[numOfGoals] = {32,76,120,164,208};
volatile uint8_t goalReached[numOfGoals] = {0,0,0,0};
volatile uint8_t lives = 3;
volatile uint16_t score = 0;
volatile uint8_t fps = 0;
volatile uint8_t enableCollision = 1;

typedef struct {
	uint8_t prevPos;
	uint8_t pos;
} vehicle;

typedef struct {
	uint8_t hiTrack;
	uint8_t track;
	uint8_t prevTrack;
	uint8_t x;
	uint8_t prevX;
} frog;

const int8_t roadLaneSpeed[numRoadLanes] = {1,-2,3,1,-2,0,-1,3,-2,2,1};
const uint8_t vehicleLength[numRoadLanes] = {2*unitLength,2*unitLength,1*unitLength, 1*unitLength,1*unitLength,0,2*unitLength,2*unitLength,1*unitLength, 1*unitLength,1*unitLength};
const uint16_t laneColours[numRoadLanes] = {BURLY_WOOD,RED,BURLY_WOOD,BURLY_WOOD,RED,BLACK,GOLD,GREEN_YELLOW,PALE_TURQUOISE,CRIMSON,LAVENDER};

const frog defaultFrog = {14,14,13,120+frogSize/2,120+frogSize/2};

volatile vehicle roadLanes[numRoadLanes][maxVehiclesPerLane] = {
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}},
	{{10,10},{90,90},{160,160}},
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}},
	{{10,10},{90,90},{160,160}},
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}},
	{{40,40},{90,90},{150,150}}
};

volatile frog mainFrog;

volatile int8_t collided =0;


#define laneTopOffset 5
#define laneBottomOffest 12

void drawVehicles(){
	//Draw road vehicles
	for(uint8_t lane = 0; lane<numRoadLanes;lane++){
		uint16_t currentlane = (roadOffset + lane -1) * trackWidth + offsetPx + laneTopOffset;
		uint8_t size = vehicleLength[lane];
		int8_t vSpeed = roadLaneSpeed[lane];

		for (uint8_t ve = 0; ve<maxVehiclesPerLane;ve++){
			vehicle vex = roadLanes[lane][ve];
			//Moving Right
			if (vSpeed > 0 ){
				//Moved off right side of screen
				if (vex.pos < vex.prevPos){
					roadLanes[lane][ve].prevPos = vex.pos;
				}
				//Moving right
				else if (vex.pos > vex.prevPos){
					rectangle newR = {vex.prevPos,vex.pos,currentlane,currentlane + laneBottomOffest};
					rectangle oldR = {vex.prevPos-size,vex.pos-size,currentlane,currentlane + laneBottomOffest};
					fill_rectangle(newR,laneColours[lane]);
					fill_rectangle(oldR,colours[(roadOffset + lane -1)]);
					roadLanes[lane][ve].prevPos = vex.pos;
				}
				//If Should still be visible tale on right side. (This is faked so will not align with hitbox)
				if (vex.pos < size){
					uint8_t y = 255-(size-vex.pos)+1;
					uint8_t z = 255-(size-vex.prevPos);
					if (y<screenWidth){
						rectangle oldR = {z,y,currentlane,currentlane + laneBottomOffest};
						fill_rectangle(oldR,colours[(roadOffset + lane -1)]);
					}
				}
			}
			else{ //Moving Left
				//Moving of left side of screen
				uint8_t leftPos = vex.pos-size;
				uint8_t leftPrevPos = vex.prevPos-size;
				if (leftPos > leftPrevPos){
					roadLanes[lane][ve].prevPos = vex.pos;
				}
				//Moving right
				else if (leftPos < leftPrevPos){
					rectangle newR = {vex.pos-size, vex.prevPos-size,currentlane,currentlane + laneBottomOffest};
					rectangle oldR = {vex.pos,vex.prevPos,currentlane,currentlane + laneBottomOffest};
					fill_rectangle(newR,laneColours[lane]);
					fill_rectangle(oldR,colours[(roadOffset + lane -1)]);
					roadLanes[lane][ve].prevPos = vex.pos;
				}
				//If Should still be visible tale on right side. (This is faked so will not align with hitbox)
				if (vex.pos < size){
					uint8_t y = 255-(size-vex.pos)+1;
					uint8_t z = 255-(size-vex.prevPos);
					if (y<screenWidth){
						rectangle newR = {z,y,currentlane,currentlane + laneBottomOffest};
						fill_rectangle(newR,laneColours[lane]);
					}
				}

				
			}
			
			 
		}

	}
}


void drawFrog(){
	if(mainFrog.prevTrack!=mainFrog.track || mainFrog.prevX!=mainFrog.x){
		int16_t currentlane = mainFrog.track * trackWidth -5;
		int16_t previouslane = mainFrog.prevTrack * trackWidth -5;
		rectangle newR = {mainFrog.x-frogSize,mainFrog.x,currentlane,currentlane+12};
		rectangle oldR = {mainFrog.prevX-frogSize,mainFrog.prevX,previouslane,previouslane+12};
		fill_rectangle(newR,frogColour);
		fill_rectangle(oldR,colours[mainFrog.prevTrack-1]);
		mainFrog.prevTrack=mainFrog.track;
		mainFrog.prevX=mainFrog.x;
	}
	//If collision is disabled draw frog every refresh.
	else if(!enableCollision) {
		int16_t currentlane = mainFrog.track * trackWidth -5;
		rectangle newR = {mainFrog.x-frogSize,mainFrog.x,currentlane,currentlane+12};
		fill_rectangle(newR,LIME_GREEN);
	}
}


void drawRoads(){
	//Draw Lanes
	for (uint8_t x = 0;x < numTracks; x+=1){
		rectangle r = {0,display.width,x*trackWidth+offsetPx,(x+1)*trackWidth+offsetPx};
		fill_rectangle(r,colours[x]);
	}
	//Draw Goals
	drawGoals();
	//Draw top border.
	rectangle r = {0,display.width,(goalTrack-1)*trackWidth+offsetPx+20,(goalTrack)*trackWidth+offsetPx};
	fill_rectangle(r,colours[goalTrack]);
}


void drawGoals(){
	for (uint8_t x = 0; x<numOfGoals; x+=1){
		rectangle r = {goalPositions[x]-(unitLength/2),goalPositions[x]+(unitLength/2),goalTrack*trackWidth+offsetPx,(goalTrack+1)*trackWidth+offsetPx};
		fill_rectangle(r,goalBGColour);
		if (goalReached[x]){
			int16_t currentlane = (goalTrack+1) * trackWidth -5;
			r = (rectangle){goalPositions[x]-frogSize/2,goalPositions[x]+frogSize/2,currentlane,currentlane+12};
			fill_rectangle(r,frogColour);
		}
	}
}


void updateVehicles(){
	//Relies on 8 bit overflow to loop
	for(uint8_t lane = 0; lane<numRoadLanes;lane++){
		uint8_t speed = roadLaneSpeed[lane];
		for (uint8_t ve = 0; ve<maxVehiclesPerLane;ve++){
			roadLanes[lane][ve].pos += speed;
		}
	}
}

void collision(){
	if (enableCollision && roadOffset<=mainFrog.track && mainFrog.track<numTracks){ //If on road
		for (uint8_t ve = 0; ve<maxVehiclesPerLane;ve++){
				vehicle vex = roadLanes[mainFrog.track-roadOffset][ve];
				if (
					(mainFrog.x < vex.pos  &&  mainFrog.x-frogSize > vex.pos-vehicleLength[mainFrog.track-roadOffset])
					||
					(mainFrog.x-frogSize < vex.pos  &&  mainFrog.x-frogSize > vex.pos-vehicleLength[mainFrog.track-roadOffset])
					||
					(mainFrog.x > vex.pos-vehicleLength[mainFrog.track-roadOffset]  &&  mainFrog.x < (vex.pos))
				){
					deathHandler();
					break;
				}
		}
	}
}

void deathHandler(){
	lives-=1;
	resetFrog();
	drawStats();
}

void resetFrog(){
	mainFrog.x = defaultFrog.x;
	mainFrog.track = defaultFrog.track;
	mainFrog.hiTrack = defaultFrog.hiTrack;
	drawFrog();
	drawVehicles();
}

void drawStats(){
	char buffer[4];

	sprintf(buffer, "%03d", lives);
	display_string_xy("Lives:", 20, 0);
	display_string_xy(buffer, 60, 0);

	sprintf(buffer, "%03d", score);
	display_string_xy("Score:", 80, 0);
	display_string_xy(buffer, 120, 0);

	sprintf(buffer, "%01d", enableCollision);
	display_string_xy("Collision:", 150, 0);
	display_string_xy(buffer, 210, 0);
}

void putText(uint8_t t, uint16_t x, uint16_t y){
	char buffer[4];
	sprintf(buffer, "%03d", t);
	display_string_xy(buffer, x, y);
}

ISR(INT6_vect)
{
	
	drawVehicles();
	drawFrog();
	fps++;
}

//Movement
ISR(TIMER1_COMPA_vect){	
	if (center_pressed()){
		enableCollision=!enableCollision;
		drawStats();
	}
	if(left_pressed() && mainFrog.x>frogStepX){
		mainFrog.x-=frogStepX;
	}
	if(right_pressed() && mainFrog.x<screenWidth-frogStepX){
		mainFrog.x+=frogStepX;
	}
	if(up_pressed() && mainFrog.track>2){
		//If just below goal row check that the space is free.
		if (mainFrog.track==goalTrack+2){
			for (uint8_t x = 0; x<numOfGoals; x+=1){
				if (!goalReached[x] && goalPositions[x]-(unitLength/2)<mainFrog.x  && goalPositions[x]+(unitLength/2)>mainFrog.x){
					goalReached[x] = 1;
					drawGoals();
					resetFrog();
					score+=100;
					drawStats();
				}
			}
		}
		else{
			mainFrog.track-=1;
			if (mainFrog.track<mainFrog.hiTrack){
				score+=scoreMovement;
				mainFrog.hiTrack = mainFrog.track;
				drawStats();
			}
		}
	}
	if(down_pressed() && mainFrog.track<numTracks){
		mainFrog.track+=1;
	}
	updateVehicles();
	collision();
}

ISR(TIMER3_COMPA_vect)
{
	drawStats();

	char buffer[4];
	sprintf(buffer, "%03d", fps);
	display_string_xy(buffer, 0, 0);
	fps = 0;
}

void main() {

	//Set clock to max
	CLKPR = (1 << CLKPCE);
    CLKPR = 0;
	// Set up LCD
	init_switches();
	init_lcd();
	set_frame_rate_hz(61);
	set_orientation(North);
	/* Enable tearing interrupt to get flicker free display */
	EIMSK |= _BV(INT6);

	/* Enable performance counter (Timer 3 CTC Mode 4) */
	TCCR3A = 0;
	TCCR3B = _BV(WGM32);
	TCCR3B |= _BV(CS32);
	TIMSK3 |= _BV(OCIE3A);
	OCR3A = 31250;

	/* Enable game timer interrupt (Timer 1 CTC Mode 4) */
	TCCR1A = 0;
	TCCR1B = _BV(WGM12);
	
	TIMSK1 |= _BV(OCIE1A);

	TCCR1B |= _BV(1);  //Timer Scale
	OCR1A = 31250; //Timer trigger value
	//TCCR1B |= _BV(2);
	//OCR1A = 2000;

	while (1){
		collided = 0;
		mainFrog = defaultFrog;
		drawRoads();
		drawStats();
		sei();
		while (lives!=0){}
		cli();
		drawFrog();
		display_string_xy("GAME OVER", 100, 50);
		while(!center_pressed()){lives=3;}
	}
	

}


