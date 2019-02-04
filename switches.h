//GPL-3.0 https://github.com/EpicPhill/FortunaTetris
//https://raw.githubusercontent.com/EpicPhill/FortunaTetris/master/switches.h

#define SWN     PC2
#define SWE     PC3
#define SWS     PC4
#define SWW     PC5
#define SWC     PE7

#define COMPASS_SWITCHES (_BV(SWW)|_BV(SWS)|_BV(SWE)|_BV(SWN))

void init_switches();
int center_pressed();
int left_pressed();
int right_pressed();
int up_pressed();
int down_pressed();
int down_held();