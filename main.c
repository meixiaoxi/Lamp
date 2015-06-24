/*  HR7P159P2D
           ----------------------------------------------
VDD     	--|VDD                                      VSS|-- VSS
KEY    	--|PA3/KIN3/OSC1/CLKI     PB0/PINT0/PWM2/ISPSDA|-- SDA
LOAD_CTL --|PA2/KIN2/OSC2/CLKO           PB1/KIN4/ISPSCK|-- SCL
OUT_CTL  --|PA1/KIN1/RST/VPP        PA0/KIN0/PWM1/T8NCKI|-- PWM
           ----------------------------------------------
*/

#include <hic.h>
#include "lamp.h"

sbit gLedWave;   //��¼��ʱ��led���ȱ仯����
unsigned char gLedStrength;    //��ǰled����ֵ
unsigned short g3STick;   // 3S �ļ���
unsigned char gSysReadPA;
unsigned char gLampMode;


unsigned long gCrcCode;
unsigned char gLedStatus;

#ifdef HYPNOSIS_MODE_SUPPORT
unsigned char gHypnosisDownLevel;
unsigned char gHypnosisDownCount;
unsigned char gCandleIntervalBuf[7] = {61,120,100,250,90,180,50};   // 61==1s
unsigned char gCandleTypeBuf[7] = {0,2,0,1,2,0,1};
unsigned char gCandleShakePos;
#endif 

#define u8 unsigned char
#define _nop_() __Asm	 NOP
#define LoadCtl 			 PA2

#define EnWatchdog()		RCEN = 1
#define DisWatchdog()	RCEN = 0

#define key_interrupt_enable()	KIE = 1,KMSK1 = 1
#define key_interrupt_disable()	KIE = 0,KMSK1 = 0

#define pwm_stop()    T8P1M = 0  //�ر�PWM
#define pwm_start()   T8P1M = 1  //����PWM


//���ڶ�ʱ�رչ��ܺʹ���ģʽ�µļ�ʱ
#define t8n_start()	T8NEN = 1
#define t8n_stop()	T8NEN = 0


//T8P2 ,ʹ���䶨ʱ�����ܣ�LOAD_CTL��3s ��factory reset��4s
#define t8p2_start()	T8P2E = 1    
#define t8p2_stop()	T8P2E = 0




#ifdef BREATHE_MODE_SUPPORT
unsigned char gBreatheCount;
sbit gBreatheWave;
#endif


void isr(void) interrupt
{
    	if(KIE && KIF) 
    	{
		gSysReadPA = PA;			//����жϱ�־ǰ���Զ˿ڲ���һ��
		KIF = 0;  		//���KINT�ⲿ�����жϱ�־
    	}
   	 if(T8P2IE&&T8P2IF)        //����T8P2��ʱ���ж�		
    	{					        
        	T8P2IF=0;		    //��T8P2�жϱ�־
       	 g3STick++;
    	}
	if(T8NIE && T8NIF)
    	{
    		T8NIF = 0;
/*
		gCountCHAR++;
		if(gCountCHAR == 0xFF)
			gCountINT++;
			*/
    	}

	#if 0
	if(T8P1IE && T8P1IF)
	{
		T8P1IF = 0;

		gSoftPWMCount++;
		if(gSoftPWMCount >=20)
		{
			PA0 =1;
			gSoftPWMCount = 0;
		}
		if(gSoftPWMCount ==1)
			PA0 = 0;
	}
	#endif
	
}


void	Wait_uSec(unsigned int DELAY)
{
      unsigned int jj;                            //

      jj= 2*DELAY;                          // by 16MHz IRC OSC
      
      while(jj)  jj--;                    //
}
void delay_ms(unsigned int count)
{
	unsigned char ii;	
	while(count--)
	{
		for(ii =0 ;ii < 250; ii++);
		__Asm CWDT;
	}
}

#if 0
void LoadCtlDetect()
{
	if(g3STick >= LOAD_CTL_TICK)
	{
		g3STick =0;
		if(T8P1RL <=40)
		{
			LoadCtl = 1;
			delay_ms(/*250*//*300*//*400*/600);   //100 150 200
			LoadCtl = 0;
		}
          	//CandleShake();
	}
}
#endif
#ifdef HYPNOSIS_MODE_SUPPORT
void EnterHypnosisMode()
{
	gLampMode = HYPNOSIS_MODE;
	gCandleShakePos = 0;
	gHypnosisDownCount = 30;
	gHypnosisDownLevel = 7;
	pwm_stop();
	T8P1RL = gLedStrength = HYPNOSIS_STRENGTH;
	pwm_start();
	key_interrupt_enable();
	gSysReadPA = 0xAB;
}
#endif

#ifdef BREATHE_MODE_SUPPORT
void EnterBreatheMode()
{
	gLampMode = BREATHE_MODE;
	gBreatheCount =0;
	gBreatheWave = BREATHE_UP;
	T8P1RL = 5;
	gLedStrength = 5;
	key_interrupt_enable();
	gSysReadPA = 0xAB;
}
#endif

#define POWER_ON	0
#define POWER_OFF	1
#define POWER_NIGHT	2


void EnterNightMode()
{
	LoadCtl =1;
	//SlowChangeStrength(POWER_NIGHT);
	pwm_stop();
	T8P1RL = LED_MIN_LEVEL;
	pwm_start();
	PA3 = 0;
	gLampMode = NIGHT_MODE;
}


void SlowChangeStrength(unsigned char type)
{

	unsigned char temp = 0;
	unsigned char count = 0;


	if(gLedStrength ==  LED_MIN_LEVEL-1)
		PA3 = 0;
	  
		if(type == POWER_ON)
		{
			while(1)
			{
				if(temp > gLedStrength)
					break;
				T8P1RL  = temp;
				if(temp == 255)
					break;
				
				temp++;

				if(temp < 70)
					delay_ms(60);
				else
					delay_ms(30);
				
				
				//delay_ms(20);
				if(P_KEY == 0)
				{	
					count++;
				}
				else if(count == 0)
				{
					continue;
				}
				else 
				{
					if(count>=2 && count <50)   //short key press
					{
						//enter Сҹ��ģʽ
						//EnterNightMode();
						return;
					}
					count =0;	
				}
			}

		}
		else if(type == POWER_OFF)
		{
			temp = T8P1RL;
			
			while(1)
			{
				if(type == POWER_OFF)
				{
					if(temp == 0)
						break;
				}
				else
				{
					if(temp==LED_MIN_LEVEL-1)
						break;
				}
				pwm_stop();
				T8P1RL = temp--;
				pwm_start();

				delay_ms(10);


				if(P_KEY == 0)
				{	
					count++;
				}
				else if(count == 0)
				{
					continue;
				}
				else
				{
					if(count>=2 && count <50)   //short key press
					{
						/*
						if(type == POWER_NIGHT)
						{
							#if defined(BREATHE_MODE_SUPPORT)
							EnterBreatheMode();
							#elif defined(HYPNOSIS_MODE_SUPPORT)
							EnterHypnosisMode();
							#endif
							return;
						}
						*/
					}
				}
			/*
				if(temp < 50)
				{
					delay_ms(40);
					temp --;
				}
				else
				{
					temp = temp -4;
					delay_ms(1);
				}
			*/
/*				if(PA3 == 0)
				{	
					delay_ms(10);
					if(PA3 == 0)
						return;
				}
				*/
			}
		}
}

void factoryReset()
{
	unsigned char temp=0;
	I2C_write(ADDR_STRENGTH_FLAG, 0x00);   //clear our flag
	GIE =0;
	
	T8P1RL = 150;
  	T8P1E = 1;

	do{
		delay_ms(300);
		pwm_start();
     		delay_ms(300);
     		pwm_stop();
		temp++;
	   }while(temp<3);
	while(1)
		__Asm IDLE;
}

void LampPowerOFF()
{
//	pwm_stop();
	LoadCtl = 0;
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	PA0 = 0;
	
	if(gLedStrength<LED_MIN_LEVEL-1)
		gLedStrength = LED_DEFAULT_LEVEL;
	DisWatchdog();

	I2C_write(ADDR_ONOFF_FLAG, LED_NOW_OFF);
    	SlowChangeStrength(POWER_OFF);
	
	pwm_stop();
	PA3 = 1;
	T8P1RL = 0;
	while(P_KEY==0){};
	//delay_ms(5);
	key_interrupt_enable();
	__Asm NOP;
	__Asm IDLE;		
	
	key_interrupt_disable();
	
	DisWatchdog();

	g3STick =0;
	t8p2_start();
	while(P_KEY == 0) //wait key release
	{
		if(g3STick > 183)    //   3s/16.384ms  factoryReset();
 		{
			factoryReset();
		}
	}
	t8p2_stop();
	
	
	EnWatchdog();
	
	pwm_start();

	I2C_write(ADDR_ONOFF_FLAG, LED_NOW_ON);
    	SlowChangeStrength(POWER_ON);

/*
	if(T8P1RL != gLedStrength)
	{
		EnterNightMode();
	}
	*/
	if(T8P1RL <=PWM_NUM_START_LOAD)
	{
		LoadCtl = 1;
	}
	
/*
	while(1)  	//��������
	{
		T8P1RL  = temp++;
		if(temp > gLedStrength)
			break;
		delay_ms(20);
	}
*/

/*
	while(PA3==0)  //wait key to release
	{
		__Asm CWDT;
	}
	*/
}


#ifdef HYPNOSIS_MODE_SUPPORT
void CandleShake(unsigned char type)
{
	unsigned char time;
	unsigned char temp = gLedStrength;

	unsigned char interval = 150;



	if(type == 3)
		interval = 60;

  again:	
		time = 0;
		do{
		pwm_stop();
		if(gLedStrength >1)
			gLedStrength -= 1;
		T8P1RL = gLedStrength;
		pwm_start();
		delay_ms(interval);
		time++;	
		}while(time<=5);
	
	time = 0;
	#if 0
	pwm_stop();
	delay_ms(5);
	pwm_start();	
	#endif
	
	do{
		pwm_stop();
		if(gLedStrength != temp)
			gLedStrength += 1;
		T8P1RL = gLedStrength;
		pwm_start();
		delay_ms(interval);
		time++;
		}while(time<=5);

	if(type == 1)
	{
		type =0;
		interval = 50;
		goto again;
	}
}

#endif


unsigned char levelCount =0;
void changeLampStrength()
{
	unsigned char temp = 0;
	if(gLedWave == LED_STRENGTH_UP)
	{
		PA3 =1;
		if(gLedStrength != 255)
			gLedStrength = gLedStrength + 1;
		else
		{
			#if 1
				//	gLedStrength = gLedStrength -1;
					do{
						pwm_stop();
						delay_ms(400);
						pwm_start();
     						delay_ms(400);
						temp++;
					}while(temp<3);
					gLedWave = LED_STRENGTH_DN;
			#else
			if(levelCount == 0)
			{
				//led_blink();
					do{
						pwm_stop();
						delay_ms(500);
						pwm_start();
     						delay_ms(500);
						temp++;
					}while(temp<4);
			}
			levelCount++;

			gLedStrength = gLedStrength -1;
			if(levelCount > 30)
			{
				levelCount =0;
				gLedWave = LED_STRENGTH_DN;
			}
			#endif
		}		
	}
	else
	{
		if(gLedStrength != LED_MIN_LEVEL -1)
			gLedStrength = gLedStrength - 1;
		else
		{
			#if 1
					//gLedStrength = LED_MIN_LEVEL;
					PA3 = 0;
					do{
						pwm_stop();
						delay_ms(400);
						pwm_start();
     						delay_ms(400);
						temp++;
					}while(temp<3);

					gLedWave = LED_STRENGTH_UP;
			#else					
			if(levelCount == 0)
			{
				//led_blink();
					do{
						pwm_stop();
						delay_ms(500);
						pwm_start();
     						delay_ms(500);
						temp++;
					}while(temp<4);
							
			}
		
			gLedStrength = LED_MIN_LEVEL;
				
			levelCount++;
			if(levelCount > 30)
			{
				levelCount =0;
			//	PA3 =0;
				gLedWave = LED_STRENGTH_UP;
			}
			#endif
		}	
	}

	pwm_stop();
	T8P1RL  = gLedStrength;//gStrengthBuf[gLedStrength];
	pwm_start();
	if(T8P1RL<=PWM_NUM_START_LOAD)
	{
		LoadCtl=1;
	}	
	else
		LoadCtl=0;
}



void delay_with_key_detect()
{
	int mCount = 0;
	unsigned char isLongPress = 0;

	delay_ms(10);
	if(P_KEY != 0)  //��������
		return;
	while(1)
	{
		delay_ms(10);
		if(P_KEY == 1)   //key is released
			break;
		mCount++;
	
		if(isLongPress == 0)
		{
			if(mCount>=100)    // 1s
			{
				if(gLampMode != ADJUST_MODE)
					return;			
				isLongPress = 1;
				mCount =0;
				changeLampStrength();
			}
		}
		else
		{
			if(gLedStrength >50)
			{
				if(mCount >= 4)  //10ms
				{
					mCount = 0;
					changeLampStrength();
				}
			}
			else
			{
				if(mCount >= 10)  //100ms
				{
					mCount = 0;
					changeLampStrength();
				}
			}
		}

	//	LoadCtlDetect();

	}

	levelCount = 0;
	
	if(isLongPress == 0)   //short press
	{
		/*
		if(gLampMode == ADJUST_MODE)
		{
			//enter Сҹ��ģʽ
			EnterNightMode();
			return;
		}
		*/
		
		//LampPowerOFF();

		while(1)
		{
			LampPowerOFF();
			if(T8P1RL <=PWM_NUM_START_LOAD)
			{
				LoadCtl = 1;
			}
			if(T8P1RL == gLedStrength)
				break;
		}
		return;
	}

/*
	if(gLampMode ==  ADJUST_MODE)  //clear
	{
		gCountCHAR =0;
		gCountINT =0;
	}
*/
	// lamp strength has been changed, we need save
	I2C_write(ADDR_STRENGTH,gLedStrength);

	//return LED_ON;
}

void InitConfig()
{
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	 gLedWave = LED_STRENGTH_DN;
	//KIN mask
	//initial value, code space
	/*
	KMSK0 = 0;
	KMSK2 = 0;
	KMSK4 = 0;

	KMSK3 = 0;  //key
	*/
    	KIE = 0;   
    	
	//LoadCtl��
	PAT2 = 0; // ���
	PA2 = 0;   //�͵�ƽ

	//res_ctl ��
	PAT3 = 0; //���
	PA3 = 1;   //���貢��

	//i2c������
	PBT = 0x00;
	PB  = 0x00;
	
	//PWM������
	//����
	PA0 = 0;    
	PAT0 = 0;
	
	T8P1P = 254;		//����PWM���ڼĴ�����ֵ10KHz
	T8P1RL = 0x00;
	T8P1OC = 0x00; 		//����PA0���PWM
	T8P1C = 0x04;		//����T8P1Ԥ��Ƶ1:1�����Ƶ��Ч��/*������T8P1	*/

	//�ر�PWM�ж�
	/*
	T8P1IE = 0;   
	T8P2IE = 0;
	T8P1IF = 0;
	T8P2IF = 0; 
	*/

	//T8N, ���ڴ���ģʽ��ʱ�Ͷ�ʱ�ر�
	//���μ���ʱ��  256*256*2/(8*10^6)) = 16.384ms
	T8NC = 0x07;	    //����T8NΪ��ʱ��ģʽ����Ƶ��Ϊ1:256
       T8N	= 0;			//�趨ʱ����ֵ
       T8NPRE = 1;			//ʹ��Ԥ��Ƶ��

	T8NIF=0;
       T8NIE = 1;			//ʹ��T8N��ʱ���ж�
       T8NEN = 1;			//��T8NEN��ʱ��

	//T8P2,  ʹ���䶨ʱ��ģʽ������LOAD_CTL����
	//һ�μ���ʱ��   16*16*(2*/(8*10^6))*256 =16.384ms
	T8P2 = 0;  //clear count reg
	T8P2P = 0xFF;
	T8P2C = 0x7B;   //��ʱ��ģʽ��Ԥ��Ƶ��1:16, ���Ƶ��1:16
	T8P2IF = 0;
	T8P2IE = 1;

	//watchdog
	WDTC = 0x17;   //���ʱ��2s
	
	GIE = 1;		    //ʹ��ȫ���ж�
}

void main()
{
	unsigned char mDownCount;
    	OSCP = 0x55;		//ʱ�ӿ���д��������
    	OSCC = 0xE0;	    //�л�������ʱ�ӣ�8MHz��
        
    	while (!SW_HS) __Asm CWDT;    //�ȴ�����ʱ���л����

	
        
   	InitConfig();    //��ʼ������

	DisWatchdog();

	//t8n_stop();

        gLedStatus = I2C_read(ADDR_ONOFF_FLAG);
		
	
	if(gLedStatus == LED_PRE_ON)
	{
		I2C_write(ADDR_ONOFF_FLAG,LED_NOW_OFF);
		key_interrupt_enable();
		__Asm IDLE;
		key_interrupt_disable();

		DisWatchdog();   //�����жϻ���֮��ϵͳ��Ĭ�Ͻ�RCEN��1
		g3STick = 0;
		t8p2_start();
		while(P_KEY == 0) //wait key release
		{
			if(g3STick > 183)    //   3s/16.384ms  factoryReset();
 			{
				factoryReset();
			}
		}
		t8p2_stop();
	//	g3STick =0;
	}


	EnWatchdog();
	
	//check whether strength exists, if not, use default strength
	gLedStrength = I2C_read(ADDR_STRENGTH_FLAG);
	if(gLedStrength == 0xAB)  //ok, it's our flag
	{
		//delay_ms(5);
		gLedStrength = I2C_read(ADDR_STRENGTH);
	}
	else
	{
		I2C_write(ADDR_STRENGTH_FLAG, 0xAB);  //write our flag
		gLedStrength = LED_MAX_LEVEL;  //max level
		//delay_ms(5);
		I2C_write(ADDR_STRENGTH,254);
	}

	//�ߴ���	
   if(gLedStrength < LED_MIN_LEVEL-1)
   {
   	gLedStrength = LED_DEFAULT_LEVEL;
   }

  	 pwm_start();

	I2C_write(ADDR_ONOFF_FLAG,LED_NOW_ON);
	SlowChangeStrength(POWER_ON);

	while(1)
	{
		if(T8P1RL <=PWM_NUM_START_LOAD)
		{
			LoadCtl = 1;
		}
		if(T8P1RL != gLedStrength)
		{	
			LampPowerOFF();
			if(T8P1RL == gLedStrength)
				break;
		}
		else
			break;
		
	}
	/*
	if(T8P1RL != gLedStrength)
	{
		EnterNightMode();
	}
	*/
	
	//main loop
    	while(1)
    	{
		if(P_KEY == 0)   //key press
		{
			delay_with_key_detect();
		}


/*		
		 if(gLampMode == ADJUST_MODE) 
		{
			if(gCountINT >= 2647)      // 3 Hour  3*60*60*1000/16.384/255 = 2647
				LampPowerOFF();
		}
*/
		//LoadCtlDetect();


		__Asm CWDT;
    	}
}
			
