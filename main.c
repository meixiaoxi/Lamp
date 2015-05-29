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
//sbit isConsecutivePress;  //连续按键侦测标记
sbit gLedWave;   //记录此时的led亮度变化趋势
unsigned char gLedStrength;    //当前led亮度值
unsigned short g3STick;   // 3S 的计数
unsigned char gSysReadPA;
unsigned int gCountINT;
unsigned char gCountCHAR;
unsigned char gLampMode;

unsigned char mTemp = 0;

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

#define pwm_stop()    T8P1M = 0  //关闭PWM
#define pwm_start()   T8P1M = 1  //启动PWM


//用于定时关闭功能和催眠模式下的计时
#define t8n_start()	T8NEN = 1
#define t8n_stop()	T8NEN = 0


//T8P2 ,使用其定时器功能，LOAD_CTL的3s 和factory reset的4s
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
	gCountCHAR =0;
	gCountINT = 1831;
	gHypnosisDownCount = 30;
	gHypnosisDownLevel = 7;
	pwm_stop();
	T8P1RL = gLedStrength = HYPNOSIS_STRENGTH;
	pwm_start();
	mTemp = gCountCHAR;
	key_interrupt_enable();
	gSysReadPA = 0xAB;
}
#endif

#ifdef BREATHE_MODE_SUPPORT
void EnterBreatheMode()
{
	gLampMode = BREATHE_MODE;
	gBreatheCount =0;
	gCountCHAR =0;
	gCountINT = 1831;
	gBreatheWave = BREATHE_UP;
	T8P1RL = 5;
	gLedStrength = 5;
	mTemp = 0;
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
	SlowChangeStrength(POWER_NIGHT);
	PA3 = 0;
	gLampMode = NIGHT_MODE;
}


void SlowChangeStrength(unsigned char type)
{

	unsigned char temp = 0;
	unsigned char count = 0;
	  
		if(type == POWER_ON)
		{
			while(1)
			{
				if(temp > gLedStrength)
					break;
				T8P1RL  = temp++;

				/*
				if(temp < 70)
					delay_ms(30);
				else
					delay_ms(30);
				*/
				delay_ms(20);
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
						//enter 小夜灯模式
						//EnterNightMode();
						return;
					}
					count =0;	
				}
			}

		}
		else if(type == POWER_OFF || type == POWER_NIGHT)
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

				delay_ms(20);


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
						if(type == POWER_NIGHT)
						{
							#if defined(BREATHE_MODE_SUPPORT)
							EnterBreatheMode();
							#elif defined(HYPNOSIS_MODE_SUPPORT)
							EnterHypnosisMode();
							#endif
							return;
						}
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
	I2C_write(ADDR_ONOFF_FLAG,0x00);
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
	gCountCHAR = gCountINT= 0;
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	PA0 = 0;
	gLedStrength = I2C_read(ADDR_STRENGTH);
	if(gLedStrength<LED_MIN_LEVEL|| gLedStrength>LED_MAX_LEVEL)
		gLedStrength = LED_DEFAULT_LEVEL;
	DisWatchdog();

    	SlowChangeStrength(POWER_OFF);

	gLedStatus = LED_NOW_OFF;
	
	pwm_stop();
	PA3 = 1;
	while(P_KEY==0){};
	//delay_ms(5);
	key_interrupt_enable();
	__Asm NOP;
	__Asm IDLE;		
	
	key_interrupt_disable();
	gSysReadPA = 0xAB;
	DisWatchdog();

	g3STick =0;
	while(P_KEY == 0) //wait key release
	{
		if(g3STick > 183)    //   3s/16.384ms  factoryReset();
 		{
			factoryReset();
		}
	}
	
	
	EnWatchdog();
	
	pwm_start();

	gLedStatus = LED_NOW_ON;
    	SlowChangeStrength(POWER_ON);

	if(T8P1RL != gLedStrength)
	{
		EnterNightMode();
	}
	if(T8P1RL <=40)
	{
		LoadCtl = 1;
	}
	
/*
	while(1)  	//缓慢亮灯
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



void changeLampStrength()
{
	static unsigned char count =0;

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
		if(gLedStrength  == LED_MIN_LEVEL -1)
		{
			gLedStrength = LED_MIN_LEVEL;
		//	if(PA3 == 1)
			//	PA3 = 0;   //小夜灯
			count++;
			if(count > 30)
			{
				count =0;
			//	PA3 =0;
				gLedWave = LED_STRENGTH_UP;
			}
		}	
	}

	pwm_stop();
	T8P1RL  = gLedStrength;//gStrengthBuf[gLedStrength];
	pwm_start();
	if(T8P1RL<=40)
		LoadCtl=1;
	else
		LoadCtl=0;
}



void delay_with_key_detect()
{
	int mCount = 0;
	unsigned char isLongPress = 0;

	delay_ms(10);
	if(P_KEY != 0)  //按键防抖
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

	//	LoadCtlDetect();

	}


	if(isLongPress == 0)   //short press
	{
		if(gLampMode == ADJUST_MODE)
		{
			//enter 小夜灯模式
			EnterNightMode();
			return;
		}
		//gLedPowerStatus = LED_OFF;
		else if(gLampMode == NIGHT_MODE)
		{
			#if defined(HYPNOSIS_MODE_SUPPORT)
				#ifdef CANDLE_SHAKE_DEBUG
				gLampMode = HYPNOSIS_MODE;
				gHypnosisDownCount = 1;
				gHypnosisDownLevel = 2;
				T8P1RL = gLedStrength = 7;
				mTemp = gCountCHAR;
				key_interrupt_enable();
				gSysReadPA = 0xAB;
				gCountCHAR =0;
				gCountINT = 1831;
				#else
				EnterHypnosisMode();
				#endif
			#elif defined(BREATHE_MODE_SUPPORT)
			EnterBreatheMode();
			#endif
			
			return;
			//return LED_ON;
		}	
		LampPowerOFF();
		return;
	//	return LED_OFF;
	}

	if(gLampMode ==  ADJUST_MODE)  //clear
	{
		gCountCHAR =0;
		gCountINT =0;
	}
	// lamp strength has been changed, we need save
	I2C_write(ADDR_STRENGTH,gLedStrength);

	//return LED_ON;
}

void InitConfig()
{
	g3STick = 0;
	gLampMode = ADJUST_MODE;
	gLedWave = LED_STRENGTH_UP;
	 gCountCHAR =0;
	 gCountINT= 0;
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

	//res_ctl 脚
	PAT3 = 0; //输出
	PA3 = 1;   //电阻并联

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
	T8P2P = 0xFF;
	T8P2C = 0x7B;   //定时器模式，预分频比1:16, 后分频比1:16
	T8P2IF = 0;
	T8P2IE = 1;

	//watchdog
	WDTC = 0x17;   //溢出时间2s
	
	GIE = 1;		    //使能全局中断
}

void main()
{
	unsigned char mDownCount;
    	OSCP = 0x55;		//时钟控制写保护解锁
    	OSCC = 0xE0;	    //切换到高速时钟（8MHz）
        
    	while (!SW_HS) __Asm CWDT;    //等待高速时钟切换完成

	
        
   	InitConfig();    //初始化配置

	DisWatchdog();


	if(gCrcCode != 0x51AE)
	{
		gCrcCode = 0x51AE; 
		gLedStatus = LED_NOW_ON;
	}
	else if(gLedStatus == LED_PRE_ON)
	{
		
		gLedStatus = LED_NOW_OFF;
		key_interrupt_enable();
		__Asm IDLE;
		key_interrupt_disable();

		DisWatchdog();   //按键中断唤醒之后，系统会默认将RCEN置1
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
		gLedStrength = 254;  //max level
		//delay_ms(5);
		I2C_write(ADDR_STRENGTH,254);
	}

	//冗错处理	
   if(gLedStrength > LED_MAX_LEVEL|| gLedStrength < LED_MIN_LEVEL)
   {
   	gLedStrength = LED_DEFAULT_LEVEL;
   }

  	 pwm_start();

	gLedStatus = LED_NOW_ON;
	SlowChangeStrength(POWER_ON);
	if(T8P1RL != gLedStrength)
	{
		EnterNightMode();
	}
	if(T8P1RL <=40)
	{
		LoadCtl = 1;
	}
	
	mTemp =0;
	
	//main loop
    	while(1)
    	{
		if(P_KEY == 0)   //key press
		{
			delay_with_key_detect();
		}

		#if defined( HYPNOSIS_MODE_SUPPORT)
		if(gLampMode == HYPNOSIS_MODE)   //催眠模
		#elif defined(BREATHE_MODE_SUPPORT)
		if(gLampMode == BREATHE_MODE)   //呼吸灯模式
		#endif
		{
			#if defined(HYPNOSIS_MODE_SUPPORT)
			if(gCountINT >=13)     //  1min     1*60*1000/16.384 /255=14 
			{
				 gCountINT = 0;

				if(gHypnosisDownCount ==0)
				{	
					LampPowerOFF();
					continue;
				}
				else
				{
					gHypnosisDownCount--;

					if(gLedStrength > gHypnosisDownLevel /*&& gLedStrength >= 4*/)
					{
						mDownCount = gLedStrength;
						gLedStrength = gLedStrength - gHypnosisDownLevel;

						while(mDownCount != gLedStrength)
						{
							mDownCount--;
							pwm_stop();
							T8P1RL = mDownCount;
							pwm_start();
							delay_ms(100);
						}
					}
					if(gHypnosisDownCount == 1)   //最后一级
					{
						gHypnosisDownLevel = 2;
					}
				}
			}
			else if(gHypnosisDownCount<=1 )
			{
				//for 蜡烛抖动效果
				if(gCountCHAR > mTemp)
					mDownCount = gCountCHAR - mTemp;
				else
					mDownCount = gCountCHAR  + 255 - mTemp;
				
				if( mDownCount >= gCandleIntervalBuf[gCandleShakePos & 0x07])
				{
						mTemp = gCountCHAR;
						CandleShake(gCandleTypeBuf[gCandleShakePos & 0x07]);
						gCandleShakePos++;
				}				
			}
			#elif defined(BREATHE_MODE_SUPPORT)
			if(gCountINT >=1)
			{
			if(gCountCHAR >=7)  // 120ms  -----  30s/250    250个等级分250次变化
			{
				gCountCHAR = 0;
				if(gBreatheWave == BREATHE_UP)
				{
					if(gLedStrength > 30)
						gBreatheWave = BREATHE_DN;
					else
					{
						gLedStrength++;
						T8P1RL = gLedStrength;
					}
						
				}
				else //(gBreatheWave = BREATHE_DN)
				{
					
					if(gLedStrength <=LED_MIN_LEVEL)
					{
						gBreatheWave = BREATHE_UP;
						gBreatheCount++;
						gCountINT = 0;  //clear=0;
						gCountCHAR =0;
						if(gBreatheCount>=30)
							LampPowerOFF();
					}
					else
					{
						gLedStrength--;
						T8P1RL = gLedStrength;
					}
				}
			}
			}
			#endif

			if(gSysReadPA != 0xAB)
			{
				key_interrupt_disable();
				LampPowerOFF();
			}
		}
		else if(gLampMode == ADJUST_MODE) 
		{
			if(gCountINT >= 2647)      // 3 Hour  3*60*60*1000/16.384/255 = 2647
				LampPowerOFF();
		}

		//LoadCtlDetect();


		__Asm CWDT;
    	}
}
			
