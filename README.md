﻿# LaFortunaFrog
A frogger clone for the LaFortuna.  
Currently only road section of the level is functioning. The river also functions as a road.  
The goal is to get all 5 frogs across the river to the dark green pads.

## Controls
Your current lives and score are shown at the top of the screen.  
Use the direction keys to move your frog.  
At the end of the game use the middle button to restart.

Pressing the middle button during a game will toggle collision.

## Installing
1) Install dfu-programmer, avr-gcc (avr8-toolchain)
2) Connect your LaFortuna and enable flash mode.
3) Run `make`

# Sources
lcd/* - CC Steve Gunn  
svrgb565 - MIT Copyright (c) 2015 Klaus-Peter Zauner  
switches - GPL-3.0 - https://github.com/EpicPhill/FortunaTetris 
