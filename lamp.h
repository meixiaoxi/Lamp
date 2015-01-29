#ifndef __LAMP_H__
#define __LAMP_H__

#define FOSC		16

#define LED_MAX_LEVEL	254

#define KEY_PRESS	0
#define KEY_RELEASE 1

#define LED_STRENGTH_UP	0
#define LED_STRENGTH_DN	1

//led亮/灭状态
#define LED_ON	0
#define LED_OFF	1

//lamp模式
#define ADJUST_MODE 0  //调光
#define HYPNOSIS_MODE   1 //催眠


#define LOAD_CTL_TICK	183   //   3000/16.384
#define SHORT_PRESS_TICK 	30  // 500/16.384

#define POWEROFF_SHORT_PRESS 	0
#define POWEROFF_LONG_PRESS 	1
#define LED_POWEROFF_TYPE  0   // 0 short press to poweroff
                               			     // 1  long press to poweroff

extern short I2C_read(unsigned char reg);
extern char I2C_write(unsigned char reg, unsigned char val);
#endif
