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

//sbit gLedPowerStatus;	//led的on/off
sbit gLedWave;   //记录此时的led亮度变化趋势
sbit isConsecutivePress;  //连续按键侦测标记
unsigned char gLedStrength;    //当前led亮度值
unsigned char g3STick;   // 3S 的计数
unsigned char gSysReadPA;
unsigned int gCountINT;
unsigned char gCountCHAR;
sbit gLampMode;
unsigned char gHypnosisDownLevel;
unsigned char gHypnosisDownCount;
unsigned char mTemp = 0;

#define u8 unsigned char
#define _nop_() __Asm	 NOP
#define LoadCtl 			 PA2

#define EnWatchdog()		RCEN = 0
#define DisWatchdog()	RCEN = 0

#define key_interrupt_enable()	KIE = 1,KMSK3 = 1
#define key_interrupt_disable()	KIE = 0,KMSK3 = 0

#define pwm_stop()    T8P1M = 0  //关闭PWM
#define pwm_start()   T8P1M = 1  //启动PWM


//用于定时关闭功能和催眠模式下的计时
#define t8n_start()	T8NEN = 1
#define t8n_stop()	T8NEN = 0


//T8P2 ,使用其定时器功能，LOAD_CTL的3s 和factory reset的4s
#define t8p2_start()	T8P2E = 1    
#define t8p2_stop()	T8P2E = 0




void isr(void) interrupt
{
    	if(KIE && KIF) 
    	{
		gSysReadPA = PA;			//清除中断标志前，对端口操作一次
		KIF = 0;  		//清除KINT外部按键中断标志
    	}
   	 if(T8P2IE&&T8P2IF)        //进入T8P2定时器中断		
    	{					        
        	T8P2IF=0;		    //清T8P2中断标志
       	 g3STick++;
    	}
	if(T8NIE && T8NIF)
    	{
    		T8NIF = 0;

		gCountCHAR++;
		if(gCountCHAR == 0xFF)
			gCountINT++;
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
	char ii;	
	while(count--)
	{
		for(ii =0 ;ii < 250; ii++);
		__Asm CWDT;
	}
}

void factoryReset()
{
	I2C_write(0x00, 0x00);   //clear our flag
	GIE =0;
	
	T8P1RL = 250;
   T8P1E = 1;
	do{
		delay_ms(200);
		pwm_start();
     		delay_ms(200);
      		pwm_stop();
		}while(1);
	
}

unsigned char get3STick()
{
	unsigned char tick;
	T8P2IE = 0;
	tick = g3STick;
	if(tick >=LOAD_CTL_TICK)
		g3STick =0;
	T8P2IE = 1;
	
	return tick;
}

/*
void EnableSoftPWM()
{
	pwm_stop();
	gSoftPWMCount = 0;
	T8P1 = 0;  //clear count reg
	T8P1P = 1;
	T8P1IF =0;
	T8P1RL = T8P1RH = 1;
	T8P1IE = 1;
	PA0 =1;
	GIE = 1;
		
}

void DisableSoftPWM()
{
	//pwm_start();
	T8P1IE = 0;
	PA0 = 0;
	T8P1IF =0;
	T8P1P = 0xff;
	gSoftPWMCount = 0;
}
*/
void LampPowerOFF()
{
	unsigned char mTemp;
	pwm_stop();
	gCountCHAR = gCountINT= 0;
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	PA0 = 0;
	key_interrupt_enable();
	T8P1RL = gLedStrength = I2C_read(0x01);

	DisWatchdog();
	
	__Asm IDLE;		
	_nop_();
	key_interrupt_disable();
	EnWatchdog();
	
	pwm_start();
	while(1)  	//缓慢亮灯
	{
		T8P1RL  = mTemp++;
		if(mTemp > gLedStrength)
			break;
		delay_ms(20);
		__Asm CWDT;
	}
    //	gLedPowerStatus = LED_ON;
	while(PA3==0)  //wait key to release
	{
		__Asm CWDT;
	}
}

void CandleShake(void)
{
	unsigned char time = 0;
	unsigned char temp = gLedStrength;

	
	do{
		pwm_stop();
		if(gLedStrength  > 2)
			gLedStrength -= 1;
		T8P1RL = gLedStrength;
		pwm_start();
		delay_ms(100);
		time++;	
		__Asm CWDT;
		}while(time<=3);
	
	time = 0;
	pwm_stop();
	delay_ms(5);
	pwm_start();	
	
	do{
		pwm_stop();
		if(gLedStrength < temp)
			gLedStrength += 1;
		T8P1RL = gLedStrength;
		pwm_start();
		delay_ms(100);
		time++;
		__Asm CWDT;
		}while(time<=3);


}

void changeLampStrength()
{
	static unsigned char count =0;

	if(gLampMode == HYPNOSIS_MODE)
		return;
	#if 0
	asda
	switch(gLedWave)
	{
		case LED_STRENGTH_UP:
				gLedStrength = gLedStrength + 1;
				if(gLedStrength >= LED_MAX_LEVEL)
				{
					gLedWave = LED_STRENGTH_DN;
					gLedStrength = 254;
				}
			break;
		case LED_STRENGTH_DN:
				gLedStrength = gLedStrength - 1;
				if(gLedStrength < 2)
				{	
					gLedWave = LED_STRENGTH_UP;
					//gLedStrength = 0;
				}
			break;
	}
	#else
	if(gLedWave == LED_STRENGTH_UP)
	{
		gLedStrength = gLedStrength + 1;
		if(gLedStrength >= LED_MAX_LEVEL)
		{
			count++;
			if(count<=50)   // 2s
			{
				gLedStrength = gLedStrength -1;
			}
			else
			{
				count =0;
				gLedWave = LED_STRENGTH_DN;
			}
		}		
	}
	else
	{
		gLedStrength = gLedStrength - 1;
		if(gLedStrength  == 0)
		{
			gLedStrength = 1;
			count++;
			if(count > 30)
			{
				count =0;
				gLedWave = LED_STRENGTH_UP;
			}
		}	
	}
	#endif
	pwm_stop();
	T8P1RL  = gLedStrength;//gStrengthBuf[gLedStrength];
	pwm_start();
}

void delay_with_key_detect()
{
	int mCount = 0;
	char isLongPress = 0;
	
	
	while(1)
	{
		delay_ms(10);
		if(PA3 == 1)   //key is released
			break;
		mCount++;
	
		if(isLongPress == 0)
		{
			if(mCount>=100)    // 1s
			{
				isLongPress = 1;
				mCount =0;
				changeLampStrength();
			}
		}
		else
		{
			if(gLedStrength >150)
			{
				if(mCount >= 4)  //10ms
				{
					mCount = 0;
					changeLampStrength();
				}
			}
			else
			{
				if(mCount >= 6)  //60ms
				{
					mCount = 0;
					changeLampStrength();
				}
			}
		}
		
		if(get3STick() >=LOAD_CTL_TICK)
		{
			LoadCtl = 1;
			delay_ms(150);
			LoadCtl = 0;
		}
	}

	if(isLongPress == 0)   //short press
	{				
		if(gLampMode == ADJUST_MODE)
		{
			
			gLampMode = HYPNOSIS_MODE;
		/*	if(gLedStrength < 8)
				gLedStrength = 8;
				*/
			gHypnosisDownLevel = gLedStrength >>5;   //32次下降
			if(gHypnosisDownLevel == 0)
				gHypnosisDownLevel = 1;
			 
			mTemp = gCountINT= gCountCHAR=0;   //停止3小时关闭的timer
			gHypnosisDownCount = 32; 
			return;
			//return LED_ON;
		}
		//gLedPowerStatus = LED_OFF;
		LampPowerOFF();
		return;
	//	return LED_OFF;
	}
	
	// lamp strength has been changed, we need save
	I2C_write(0x01,gLedStrength);

	//return LED_ON;
}
unsigned int temp = 98;

void InitConfig()
{
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	gLedWave = LED_STRENGTH_UP;
	gCountCHAR = gCountINT= 0;
	gHypnosisDownCount = 32;
	//KIN mask
	//initial value, code space
	/*
	KMSK0 = 0;
	KMSK2 = 0;
	KMSK4 = 0;

	KMSK3 = 0;  //key
	*/
    	KIE = 0;   
    	
	//LoadCtl脚
	PAT2 = 0; // 输出
	PA2 = 0;   //低电平

	//OUT_CTL 脚
	/*
	PA1 = 0;
	PAT1 = 1; //输入
	*/
	
	//key 脚输入
	PAT3  = 1;

	//i2c的配置
	PBT = 0x00;
	PB  = 0x00;
	
	//PWM的配置
	//周期
	PA0 = 0;    
	PAT0 = 0;
	
	T8P1P = 249;		//设置PWM周期寄存器初值10KHz
	T8P1RL = 0x00;
	T8P1OC = 0x00; 		//设置PA0输出PWM
	T8P1C = 0x04;		//设置T8P1预分频1:1，后分频无效，/*并启动T8P1	*/

	//关闭PWM中断
	/*
	T8P1IE = 0;   
	T8P2IE = 0;
	T8P1IF = 0;
	T8P2IF = 0; 
	*/

	//T8N, 用于催眠模式计时和定时关闭
	//单次计数时间  256*256*2/(8*10^6)) = 16.384ms
	T8NC = 0x07;	    //设置T8N为定时器模式，分频比为1:256
       T8N	= 0;			//设定时器初值
       T8NPRE = 1;			//使能预分频器

	T8NIF=0;
       T8NIE = 1;			//使能T8N定时器中断
       T8NEN = 1;			//打开T8NEN定时器

	//T8P2,  使用其定时器模式，用于LOAD_CTL控制
	//一次计数时间   16*16*(2*/(8*10^6))*256 =16.384ms
	T8P2 = 0;  //clear count reg
	T8P2RL = 0xFF;
	T8P2C = 0x7B;   //定时器模式，预分频比1:16, 后分频比1:16
	T8P2IF = 0;
	T8P2IE = 1;

	//watchdog
	WDTC = 0x17;   //溢出时间2s
	
	GIE = 1;		    //使能全局中断
}

void main()
{
    	OSCP = 0x55;		//时钟控制写保护解锁
    	OSCC = 0xE0;	    //切换到高速时钟（8MHz）
        
    	while (!SW_HS) __Asm CWDT;    //等待高速时钟切换完成

    	DisWatchdog();
        
   	InitConfig();    //初始化配置
	
	if(PA1 == 0)  //OUT_CTL
	{
		key_interrupt_enable();
		__Asm IDLE;
		key_interrupt_disable();

		t8p2_start();
		while(PA3 == 0) //wait key release
		{
			if(g3STick > 250)
				factoryReset();
		}
		t8p2_stop();
		g3STick =0;
	}
	t8p2_start();
	EnWatchdog();	
	//check whether strength exists, if not, use default strength
	gLedStrength = I2C_read(0x00);
	if(gLedStrength == 0xAB)  //ok, it's our flag
	{
		//delay_ms(5);
		gLedStrength = I2C_read(0x01);
	}
	else
	{
		I2C_write(0x00, 0xAB);  //write our flag
		gLedStrength = 254;  //max level
		I2C_write(0x01,gLedStrength);
	}

	//冗错处理	
   if(gLedStrength > 254)
   {
   	gLedStrength = 254;
   }
	
  	 pwm_start();
   
	while(1)  	
	{
		T8P1RL  = mTemp++;
		if(mTemp > gLedStrength)
			break;
		delay_ms(20);
		//__Asm CWDT;
	}	


	mTemp =0;

    //	EnableSoftPWM();

    	while(1)
    	{
		if(PA3 == 0)   //key press
		{
			delay_with_key_detect();
		}

		#if 0
		if(gLedPowerStatus == LED_OFF)
		{
			pwm_stop();
			PA0 = 0;
			key_interrupt_enable();
			__Asm IDLE;		
			_nop_();
			pwm_start();
			key_interrupt_disable();
			gLedPowerStatus = LED_ON;
			while(PA3==0)  //wait key to release
			{
			}
		}
		#endif
		
		if(gLampMode == HYPNOSIS_MODE)   //催眠模式
		{
			if(gCountINT >=13)     //  1min     1*60*1000/16.384 /255=14 
			{
				mTemp = gCountINT = 0;

				if(gHypnosisDownCount ==0)
				{	
					LampPowerOFF();
				}
				else
				{
					gHypnosisDownCount--;
					if(gLedStrength > gHypnosisDownLevel)
						gLedStrength = gLedStrength - gHypnosisDownLevel;
					pwm_stop();
					T8P1RL = gLedStrength;
					pwm_start();
				}
			}
			else if(gHypnosisDownCount<=2)
			{
				//for 蜡烛抖动效果
				 if( (gCountINT - mTemp) >= 2)   //8s
				{
					mTemp = gCountINT;
					CandleShake();
				}
			}
		}
		else 
		{
			if(gCountINT >= 2647)      // 3 Hour  3*60*60*1000/16.384/255 = 2647
				LampPowerOFF();
		}
		if(get3STick() >= LOAD_CTL_TICK)
		{
			LoadCtl = 1;
			delay_ms(150);
			LoadCtl = 0;
            //	CandleShake();
		}

		__Asm CWDT;
    	}
}
			
