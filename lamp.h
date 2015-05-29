#ifndef __LAMP_H__
#define __LAMP_H__



//#define BREATHE_MODE_SUPPORT

//#define HYPNOSIS_MODE_SUPPORT
//#define CANDLE_SHAKE_DEBUG

#define BREATHE_UP 0
#define BREATHE_DN 1


#define P_KEY	PA1

#define FOSC		8

#define PWM_NUM_START_LOAD		70

#define LED_MIN_LEVEL	7
#define LED_MAX_LEVEL	254
#define LED_DEFAULT_LEVEL	200

#define KEY_PRESS	0
#define KEY_RELEASE 1

#define LED_STRENGTH_UP	0
#define LED_STRENGTH_DN	1

//led ��/��״̬
#define LED_ON	0
#define LED_OFF	1

//lampģʽ
#define ADJUST_MODE 0  //����
#define HYPNOSIS_MODE   1 //����
#define BREATHE_MODE 	1 //����
#define NIGHT_MODE		2 //Сҹ��

#define LOAD_CTL_TICK		183   //   3000/16.384
	
							//488 // 8000/16.384

							//610  // 10000/16.384  
							//305		 //  5000/16.384
						
#define SHORT_PRESS_TICK 	30  // 500/16.384

#define POWEROFF_SHORT_PRESS 	0
#define POWEROFF_LONG_PRESS 	1
#define LED_POWEROFF_TYPE  0   // 0 short press to poweroff
                               			     // 1  long press to poweroff

#define HYPNOSIS_STRENGTH	210		//Ĭ�ϵĴ���ģʽ��ʼ����

//EEPROM ��ַ����
#define	ADDR_STRENGTH_FLAG	0x00   //led strengtrh �Ƿ���Ч
#define ADDR_STRENGTH 0x09		//led ����

#define  ADDR_ONOFF_FLAG	0x12    //����״̬

#define LED_PRE_ON	0x33
#define LED_NOW_ON 0x33

#define LED_NOW_OFF 0xAA
#define LED_PRE_OFF 0xAA

void SlowChangeStrength(unsigned char type);
extern short I2C_read(unsigned char reg);
extern char I2C_write(unsigned char reg, unsigned char val);
#endif
