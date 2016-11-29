#include "position_controller.h"                   
#include <xc.h>

void position_control_init(void) {
 //*******************TIMER 4 WITH INTERUPT************************
  T4CONbits.TCKPS = 2; //Prescaler N=4
  PR4 = 49999;//With prescaler of 4 give a 200 Hz signal
  TMR4 = 0;                       //reset the timer  
  T4CONbits.ON = 1;               //Turn on the timer
  IPC4bits.T4IP = 5;              //Priority level 5
  IFS0bits.T4IF = 0;              //Reset Flag
  IEC0bits.T4IE = 1;              //Enable interrupt
  //************************************************************
}

int pos_pid_control (float PKp, float PKi, float PKd, int ref_pos, int measured_pos){
  int error = ref_pos-measured_pos;
  posEint = posEint + error;
  int edot = error-posEder;
  posEder = error;

  
  int u = (PKp*error) + (PKi*posEint)+ (PKd*edot);
  if (u > 300){
	u = 300;
  }else if(u<-300){
    u = -300;
  }else{;}
  
   
  return u;
}



