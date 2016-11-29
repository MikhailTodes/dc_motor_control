#include "adc.h"                   
#include <xc.h>

void adc_init(void) {
  //**********SETUP ADC MANUAL SAMPLING AUTO CONVERSION**********
  AD1PCFGbits.PCFG0 = 0;                 //Set as ADC pin
  AD1CON1bits.SSRC = 7;                  //Auto conversion
  AD1CON1bits.ASAM = 0;                  //Manual Sampling
  AD1CON1bits.ADON = 1;                   //Turn ADC on
  //*************************************************************
}

int adc_read_count(void){
  float value = 0;
  int i = 0;
  for (i = 0; i < 5; i++){//Read a few time and average for more stable result
    //**********Read ADC into value************
    AD1CHSbits.CH0SA = 0;                // connect chosen pin to MUXA for sampling
    AD1CON1bits.SAMP = 1;                  // start sampling
    AD1CON1bits.SAMP = 0;                 // stop sampling and start converting
    while (!AD1CON1bits.DONE) {
      ;                                   // wait for the conversion process to finish
    }
    value = value + ADC1BUF0;     //save value from buffer
    //******************************************
  }
  value = value/5;  
  return (int)(value);
}


int adc_read_mA(void){//read the mA from calculated line
  int count = adc_read_count();
  //line = 0.9715x-495.0411
  float value = (0.9715*(float)count)-495.0411;
  return (int)(value);
}
