#include "current_controller.h"                   
#include <xc.h>

void current_control_init(void) {
 //*******************TIMER 2 WITH INTERUPT************************
  T2CONbits.TCKPS = 4; //Prescaler N=16
  PR2 = 999;//With prescaler of 16 give a 5 kHz signal
  TMR2 = 0;                       //reset the timer  
  T2CONbits.ON = 1;               //Turn on the timer
  IPC2bits.T2IP = 5;              //Priority level 5
  IFS0bits.T2IF = 0;              //Reset Flag
  IEC0bits.T2IE = 1;              //Enable interrupt
  //************************************************************

  //***********PWM TIMER 3 OC1 out *********************************
  T3CONbits.TCKPS = 0b000;     //Prescaler N=1
  PR3 = 3999;              //20kHz
  TMR3 = 0;                //Reset Timer 3 to 0
  OC1CONbits.OCM = 0b110;  //PWM mode without fault pin; other OC1CON bits are defaults
  OC1CONbits.OCTSEL = 1;  //Connect to timer 3
  OC1RS = 3000;             //for 75% OC1RS/(PR3+1) = 75%
  OC1R = 3000;              //75%
  T3CONbits.ON = 1;        //Turn Timer 3 on
  OC1CONbits.ON = 1;       //Turn on OC1
  //****************************************************************

  //***************DIGITAL OUTPUT PINS*************
  //pin D8 is the lucky guy (convenient location)
  TRISDbits.TRISD8 = 0;//set as output
  LATDbits.LATD8 = 0; //Set as low to begin with
  //***********************************************
}

int pi_control (float CKp, float CKi, int ref, int measured){
  int error = ref-measured;
  currEint = currEint + error;

  if (error >0){
    direction =1;
  }else{
    direction = 0;
  }
  
  int u = abs((CKp*error) + (CKi*currEint));
  if (u > 100){
	u = 100;
  }
   
  return (unsigned int)((u/100.0)*3999);
}


