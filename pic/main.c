#include "NU32.h"          // config bits, constants, funcs for startup and UART
#include "LCD.h"
#include "encoder.h"// encoder functions and init
#include "adc.h"//adc init
#include "current_controller.h"//current controller init
#include "position_controller.h" //position controller init
#include "mode_control.h"//Control the mode

//***************CONSTANTS***************
#define BUF_SIZE 200
#define REF 200
//***************************************

//**************************VARIABLES******************************
char msg[BUF_SIZE]; //LCD PRINTING
static volatile int duty = 0; //motor drive variables
static volatile float CKp = 0.3, CKi = 0.005; //Current control
static volatile float PKp = 75, PKi = 0.0, PKd = 5000.0; //position control
static volatile int currentCount = 0, ref_Wave[99], measuredCur[99]; //ITEST variables
static volatile int desiredDeg = 0;
static volatile int currRef = 0;
static volatile int traj_Index = 0, numTrajSamples = 0, trajectory_Ref[5000], measTraj[5000]; //Trajectory variables
//*****************************************************************

//********************ISR PROTOTYPES******************
void __ISR(8, IPL5SOFT) Controller(void);//Timer 2
void __ISR(16, IPL5SOFT) Pos_Controller(void);//Timer 4
//****************************************************

int main() 
{
  char buffer[BUF_SIZE];
  NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
  NU32_LED1 = 1;  // turn off the LEDs
  NU32_LED2 = 1;        
  __builtin_disable_interrupts();
  //LCD_Setup();
  encoder_init();
  adc_init();
  current_control_init();
  position_control_init();
  
  // in future, initialize modules or peripherals here
  __builtin_enable_interrupts();
  set_mode(1); //SET as IDLE to begin
  //LCD_Clear();
  //LCD_Home();
  //LCD_WriteString("Mode: IDLE");

  while(1)
    {
      NU32_ReadUART3(buffer,BUF_SIZE); // we expect the next character to be a menu command
      NU32_LED2 = 1;                   // clear the error LED
      switch (buffer[0]) {
      case 'a':                      // Read current sensor (ADC counts)
	{
	  sprintf(buffer,"%d\r\n", adc_read_count());
	  NU32_WriteUART3(buffer);  //send adc count to client
	  break;
	}
      case 'b':                      // Read current sensor (mA)
	{	
	  sprintf(buffer,"%d\r\n", adc_read_mA());
	  NU32_WriteUART3(buffer);  //send mA to client
	  break;
	}
      case 'c':                      // Encode reading option
	{	
	  encoder_counts();//read once first to clear
	  sprintf(buffer,"%d\r\n", encoder_counts());
	  NU32_WriteUART3(buffer);  //send encoder count to client
	  break;
	}
      case 'd':                      // read encoder in degrees
	{
	  encoder_counts();//read once first to clear
	  float count = encoder_counts();
	  float deg = ((count-32768)/1792)*360;// (count-32768)*(360/1792);
	  sprintf(buffer,"%f\r\n", deg);
	  NU32_WriteUART3(buffer);  //send encoder degrees to client
	  break;
	}
      case 'e':                      // Reset encoder
	{	
	  encoder_reset();//reset
	  break;
	}
      case 'f':                      // Set PWM
	{	
	  signed int n = 0;
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%d", &n);
	  if (n>0){direction = 1;}
	  if (n<0){direction = 0;}
	  duty = abs(n)*40;
	  set_mode(2); //set to PWM mode
	  break;
	}
      case 'g': //Set Current Gain
	{
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%f %f", &CKp, &CKi);
	  //LCD_Move(2,0);
	  //sprintf(msg,"CKp: %d, CKi: %d    ", CKp, CKi);
	  //LCD_WriteString(msg);
	  break;	  
	}
      case 'h': //Get Current Gains
	{
	  sprintf(buffer,"%f %f\r\n", CKp, CKi);
	  NU32_WriteUART3(buffer);  //send Kp to client
	  break;
	}
      case 'i': //Set position Gains
	{
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%f %f %f", &PKp, &PKi, &PKd);
	  //LCD_Move(2,0);
	  //sprintf(msg,"CKp: %d, CKi: %d    ", CKp, CKi);
	  //LCD_WriteString(msg);
	  break;	  
	}
      case 'j': //Get position Gains
	{
	  sprintf(buffer,"%f %f %f\r\n", PKp, PKi, PKd);
	  NU32_WriteUART3(buffer);  //send Kp to client
	  break;
	}
      case 'k': //Test Current Control
	{
	  set_mode(3);
	  while(get_mode() == 3);//Chill till Test is done
	  sprintf(buffer,"%d\r\n", 99);
	  NU32_WriteUART3(buffer);  //send number of samples to client
	  
	  int i;
	  for(i = 0; i < 99; i++){
	    sprintf(buffer,"%d %d\r\n", ref_Wave[i], measuredCur[i]);
	    NU32_WriteUART3(buffer);  //send reference and measured to client
	  }	  
	  break;
	}
      case 'l': //Go to angle (deg)
	{
	  currEint = 0;
	  posEint = 0; posEder = 0;
	  
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%d", &desiredDeg);
	  set_mode(4);//set hold mode
	  break;
	}
      case 'm': //Load Step Trajectory
	{
	  set_mode(1);//Idle while interupts are disabled
	  __builtin_disable_interrupts();
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%d", &numTrajSamples);
	  int i, temp = 0;
	  for (i = 0; i < numTrajSamples; i ++){
	    NU32_ReadUART3(buffer,BUF_SIZE);
            sscanf(buffer,"%d",&temp);
	    trajectory_Ref[i]=temp;
	  }
	  __builtin_enable_interrupts();
	  break;
	}
      case 'n': //Load Cubic Trajectory
	{
	  set_mode(1);//Idle while interupts are disabled
	  __builtin_disable_interrupts();
	  NU32_ReadUART3(buffer,BUF_SIZE);
	  sscanf(buffer, "%d", &numTrajSamples);
	  int i, temp = 0;
	  for (i = 0; i < numTrajSamples; i ++){
	    NU32_ReadUART3(buffer,BUF_SIZE);
            sscanf(buffer,"%d",&temp);
	    trajectory_Ref[i]=temp;
	  }
	  __builtin_enable_interrupts();
	  break;
      	}
      case 'o': //Excecute Trajectory
	{
	  set_mode(5);
	  while(get_mode()==5);//Chill till Trajectory is done
	  __builtin_disable_interrupts();
	  sprintf(buffer,"%d\r\n", numTrajSamples);
	  NU32_WriteUART3(buffer);  //send number of samples to client
	  
	  int i;
	  for(i = 0; i < numTrajSamples; i++){
	    sprintf(buffer,"%d %d\r\n", trajectory_Ref[i], measTraj[i]);
	    NU32_WriteUART3(buffer);  //send reference and measured to client
	  }	  
	  __builtin_enable_interrupts();
	  break;
	}
      case 'p':
	{
	  set_mode(1);//switch to IDLE
	  break;
	}
      case 'q':
	{
	  set_mode(1); //Set the mode back to IDLE state 
	  break;
	}
      case 'r':                       //get the mode
	{
	  sprintf(buffer,"%d\r\n", get_mode());
	  NU32_WriteUART3(buffer);  //send mode to client
	  break;
	}
      default:
	{
	  NU32_LED2 = 0;  // turn on LED2 to indicate an error
	  break;
	}
      }
    }
  return 0;
}

void __ISR(_TIMER_2_VECTOR, IPL5SOFT) Controller(void){//5kHz ISR

  switch (get_mode()) {
  case 1:                      // IDLE mode
    {
      //LCD_Home();
      //LCD_WriteString("Mode: IDLE    ");
      LATDbits.LATD8 = 0; //Set phase low so we know its state
      OC1RS = 0;// Set low for braking in IDLE mode
      duty = 0; //Forget previous duty so as not to start up again each time
      break;
    }
  case 2:                      // PWM mode
    {
      //LCD_Home();
      //LCD_WriteString("Mode: PWM    ");
      //LCD_Move(2,0);
      //sprintf(msg,"Duty Cycle: %d%%    ", (duty/40));
      //LCD_WriteString(msg);
      LATDbits.LATD8 = direction; //Set the direction
      OC1RS = duty; //Set the duty cycle
      break;
    }
  case 3:                      // ITEST mode
    {
      //LCD_Home();
      //LCD_WriteString("Mode: ITEST    ");
      
     
      if((currentCount>25 && currentCount<50)||(currentCount>75)){//negative wave cycle
	ref_Wave[currentCount] = (-1)*(REF);
	measuredCur[currentCount] = adc_read_mA();	
      }else{//positive wave cycle
	ref_Wave[currentCount] = (REF);
	measuredCur[currentCount] = adc_read_mA();
      }

      //***************PI CONTROL********************************
      OC1RS = pi_control(CKp, CKi, ref_Wave[currentCount], measuredCur[currentCount]);//(unsigned int)((u/100.0)*3999);
      LATDbits.LATD8 = direction; //Set the direction
      //********************************************************
      
      if (currentCount == 99){
	currentCount = 0;//Reset the counter
	set_mode(1);//Set back to IDLE mode
	currEint = 0;
	break;
      }
      currentCount+=1;//increment wave counter
      break;
    }
  case 4:                      // HOLD mode
    {
      if(currRef<0){//negative wave cycle
	direction = 0;
      }else{//positive wave cycle
	direction = 1;	
      }
      int measuredCurr = adc_read_mA();
      //***************PI CONTROL********************************
      OC1RS = pi_control(CKp, CKi, currRef, measuredCurr);//(unsigned int)((u/100.0)*3999);
      LATDbits.LATD8 = direction; //Set the direction
      //********************************************************
      break;
    }
  case 5:                      // TRACK mode
    {
      if(currRef<0){//negative wave cycle
	direction = 0;
      }else{//positive wave cycle
	direction = 1;	
      }
      int measuredCurr = adc_read_mA();
      //***************PI CONTROL********************************
      OC1RS = pi_control(CKp, CKi, currRef, measuredCurr);//(unsigned int)((u/100.0)*3999);
      LATDbits.LATD8 = direction; //Set the direction
      //********************************************************
      break;
    }
  default:
    {
      ;//Do nothing
    }
  }
 
  IFS0bits.T2IF = 0;              //Reset Flag
}


void __ISR(16, IPL5SOFT) Pos_Controller(void){//200Hz ISR
  
  if(get_mode()==4){
    encoder_counts();//read once first to clear
    float count = encoder_counts();
    float Deg = ((count-32768)/1792)*360;
    int measDeg = (int)Deg;

    //***************PID Position CONTROL********************************
    currRef = pos_pid_control(PKp, PKi, PKd, desiredDeg, measDeg);
    //*******************************************************************    
  }

  if(get_mode()==5){
    
    encoder_counts();//read once first to clear
    float count = encoder_counts();
    float Deg = ((count-32768)/1792)*360;
    int measDeg = (int)Deg;

    measTraj[traj_Index] = measDeg;
    //***************PID Position CONTROL********************************
    currRef = pos_pid_control(PKp, PKi, PKd, trajectory_Ref[traj_Index], measDeg);
    //******************************************************************* 

    if(traj_Index == numTrajSamples)
    {
      currEint = 0; posEint = 0; posEder = 0;//clear errors
      traj_Index = 0;
      desiredDeg = measDeg; //set angle to hold at
      set_mode(4);//hold mode
    }else{
      traj_Index +=1;
    }
  }
  
  IFS0bits.T4IF = 0;              //Reset Flag
}
