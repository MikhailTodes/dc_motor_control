#ifndef pos_cont
#define pos_cont


static volatile int posEint;
static volatile int posEder;//error variables

void position_control_init(void);
int pos_pid_control (float PKp, float PKi, float PKd, int ref_pos, int measured_pos);

#endif
