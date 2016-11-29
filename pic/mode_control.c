#include "mode_control.h"                   
#include <xc.h>

volatile int mode;

//1 - IDLE
//2 - PWM
//3 - ITEST
//4 - HOLD
//5 - TRACK

int get_mode(void) {
  return mode;
}

void set_mode(int x) {
  mode = x;
}
