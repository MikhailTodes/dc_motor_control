#ifndef cur_cont
#define cur_cont

volatile int currEint;
volatile int direction; //motor drive variables

void current_control_init(void);
int pi_control (float CKp, float CKi, int ref, int measured);


#endif
