#ifndef AP_TRAP_H
#define AP_TRAP_H

/* 1 when Baron is in the stage. 0 when this stage cannot spare him a slot. */
int apTrapBaron(void);
void apTrapGreasy(void);
/* 1 while The Living Daylights is replacing the gun in hand. */
int apTrapFlagHeld(void);
/* 1 when Boris is in the stage. 0 when this stage cannot spare him a slot. */
int apTrapBoris(void);

#endif
