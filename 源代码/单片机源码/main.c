/*****************************************
			   功能描述
使用左下角的按键阵列进行解锁时密码的输入撤销和删除操作，
密码长度为6，初始为 ：123456
0~9为输入，A为删除某一位，B为全部撤销输入，C为密码确认，D为关门键

现象：
输入密码前要先按任意键（1-9、A-F都可）切换到密码输入界面，否则无法输入密码
若处于密码输入界面，如果15秒没有输入任何东西便会清除所有输入进入时间显示界面，此时若要重新输入再次按任意键进入密码输入界面即可
每输入一位数字，即在数码管上从左到右依次显示
按“确认键”:
错误则清空输入，错误3次则进入倒计时，时间随错误次数的增加而递增；
正确则显示数码管显示全 “1”，且指示灯开始亮起，直到按下关门键，若
门超过 overTime 秒未关闭，则灯会闪烁（P17），且智能锁往服务端报警


*******************************************/
#include	"STC15Fxxxx.H"
#include	"RTC.h"					//实时时钟功能实现
#include	"NTC.h"		            //温度检测功能实现
#include	"display.h"				//数码管显示功能实现
#include	"STC_NET.h" 			//单片机ESP8266模块网络请求实现


#define	Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))		//Timer 0 中断频率, 1000次/秒
#define overTime    20   // 限定超过多长时间未关门，处于不正常状态

bit	B_1ms;	//1ms标志
u16	msecond;	//1000ms计数
u8	cnt50ms;	//50ms计数	 
u8	cnt10ms;	//10ms计数

u8	KeyCode;	//给用户使用的键码

u8 IO_KeyState, IO_KeyState1, IO_KeyHoldCnt;	//行列键盘变量

u8 count;		 //输入密码的个数
u8 data input[6];	//存储输入的密码变量
u8 inputcount = 0; //输入密码错误次数 范围 0~3，每3次进入倒计时，并置 0
u8 time = 0;	   //进入倒计时的次数，用于每次错误递增倒计时的时间
u8 display;  //切换显示密码、实时时间 
u8 d;	//切换显示实时时间的年月日或时分秒，隔一秒自动切换
u16 countdown_KeyCode = 0;	//用于一段时间无操作进入时间显示界面

int state = 0;      // 门锁状态，1 为开启，0为关闭，3为不正常，初始为 0
u8 clockCount = 0;	//用于计时，勿动
u8 i;			//用于循环


/*******************函数********************/
void IO_KeyScan(void);	//50ms call  by system
void count_down();       //倒计时 by ljh
void validate(); 		 //密码验证函数 by ljh
void SendState();		//获得门的状态，state 是状态位 by ljh
void Alarm(u8*);			//用于三次错误报警，放在validate() 函数中  by ljh
void SendTemperature(u16);
void UnlockSuccess();

u16 mstime = 0;
u8 stime = 0;
void main(void)
{
	u8	i;
	u8 changeCount = 0;
	u16 temperatureCount = 0;
	P0M1 = 0;	P0M0 = 0;	//设置为准双向口
	P1M1 = 0;	P1M0 = 0;	//设置为准双向口
	P2M1 = 0;	P2M0 = 0;	//设置为准双向口
	P3M1 = 0;	P3M0 = 0;	//设置为准双向口
	P4M1 = 0;	P4M0 = 0;	//设置为准双向口
	P5M1 = 0;	P5M0 = 0;	//设置为准双向口
	P6M1 = 0;	P6M0 = 0;	//设置为准双向口
	P7M1 = 0;	P7M0 = 0;	//设置为准双向口


	count = 0;

	AUXR = 0x80;	//Timer0 set as 1T, 16 bits timer auto-reload, 
	TH0 = (u8)(Timer0_Reload / 256);
	TL0 = (u8)(Timer0_Reload % 256);
	ET0 = 1;	//Timer0 interrupt enable
	TR0 = 1;	//Tiner0 run
	UART1_config(); // 串口选择波特率
	EA = 1;		//打开总中断
	P17 = 1;

	for(i=0; i<8; i++)	
	{
		input[i] = 10;	//初始化，上电消隐
		count = 0;
	}
	delay_ms(500);	//等待单片机状态稳定
	KeyCode = 0;	//给用户使用的键码, 1~12有效
	cnt10ms = 0;

	IO_KeyState = 0;
	IO_KeyState1 = 0;
	IO_KeyHoldCnt = 0;
	cnt50ms = 0; 
	display = 0;
	d = 0;
	init_time();
	WriteRTC();
	DisplayRTC(1);
	ADC_config();
	//WIFI连接初始化
	ATConnect();
    ATMode();
    ATWifi("test","test123456");
	//连接TCP服务器
    TCPConnect("172.18.32.6","3333");	
    TCPSend("0000-example");			//该锁的名称
	while(1)
	{
		if(B_1ms)	//1ms到
		{
			B_1ms = 0;
			if(++temperatureCount >= 30000){ //30s更新一次温度
				temperatureCount = 0;
				SendTemperature(get_temperature());
			}
			if(mstime++ >= 1000)
			{
				if(state == 1)
				{
					stime++;
					if(stime >= overTime) // 超时未锁门
					{
						state = 3;
						Alarm("Lock is opened for 60s\0");//报警
					}
				}
				else
				{
					stime = 0;
				}
				if(state == 3)
				{
					if( clockCount == 0)
					{
						clockCount = 1;
						P47 = 1;
					}
					else
					{
						clockCount = 0;
						P47 = 0;
					}
				}
				mstime = 0;
			}

			if(++msecond >= 1000)	//1秒到
			{
				msecond = 0;
				if(++changeCount>6){ //6秒切换一次年月日/时分秒
					changeCount=0;
					d = 1 - d;
				}
				if(display != 1)
				{
					ReadRTC();
					DisplayRTC(d);
				}
				if(BuffCMP("+IPD,16:password 123456\0")){
					if(display == 0 && state == 0)
					{
						display = 1;
					}
					for(i = 0;i < 6;i++)
					input[i] = 1;	   //密码正确，全部显示1
					count = 1;	  //尾数显示1
					inputcount = 0;
					time = 0;
					state = 1;
					DisplayPassword(input, count);
					SendState();
					UnlockSuccess();
				}
				
			}

			if(++cnt50ms >= 60)		//60ms扫描一次行列键盘
			{
				cnt50ms = 0;
				IO_KeyScan();
				if(state == 0) {
					if(KeyCode == 0)
						countdown_KeyCode++;
					if(countdown_KeyCode >= 150) 
					{
						countdown_KeyCode = 0;
						display = 0;
						count = 0;
						for(i = 0;i < 6;i++)
				    		input[i] = 10;
					}
				}
			}
			
			if((KeyCode > 0 && KeyCode <= 32))		//有键按下
			{
				if(display == 0 && state == 0)
				{
					display = 1;
				}
				else
				{
					if(KeyCode < 27 )	//输入数字0~9
					{
						if(count < 6)
						{
							input[count] = KeyCode - 17;
			    			count++;
						}
						DisplayPassword(input, count);
					}

					else if(KeyCode == 27)	 //输入回退一位
					{
			    		if(count > 0)
						{
							count--;
							input[count] = 10;
						} 
						DisplayPassword(input, count);
					}

					else if(KeyCode == 28)	//清除所有输入
					{
			    		count = 0;
						for(i = 0;i < 6;i++)
				    		input[i] = 10;
						DisplayPassword(input, count);
					}
					else if(KeyCode == 29) //确认密码
					{
						validate();
					}
					else if(KeyCode == 30) //关门
					{
						for ( i = 0; i < 7; ++i)
						{
							input[i] = 10;
						}
						count = 0;
						state = 0;
						P47 = 1;
						DisplayPassword(input, count);
						SendState();
					}				
				}
				countdown_KeyCode = 0;	
				KeyCode = 0;
			}					
		}
	}
} 

void SendState()   //状态
{
	u8 xdata temp[10];
    sprintf(temp,"0001-%d\0",state);
	TCPSend(temp);
}

void SendTemperature(u16 temperature){ //发送温度
	u8 xdata temp[10];
    sprintf(temp,"0002-%d\0",temperature);
	TCPSend(temp);
}

void Alarm(u8* msg)	//报警
{
	//每输入三次错误，就会调用这个函数，往服务端发送报警信号
	u8 xdata temp[50];
    sprintf(temp,"0003-%s\0",msg);
	TCPSend(temp);
}

void UnlockSuccess(){ //发送解锁成功消息
	u8 xdata temp[10];
    sprintf(temp,"0004-success");
	TCPSend(temp);
}

void count_down()//倒计时
{
	count = 0;
	time++;
	inputcount *= time;
	input[0] = 10;// 使无关位置变暗 
	input[1] = 10;
	input[2] = 10;
	inputcount++;
	msecond = 0;
	while(1)
	{
		delay_ms(1);	//延时1ms
		if(++msecond >= 1000)	//1秒到
		{
			inputcount--;		//倒计时秒计数-1
			msecond = 0;		//清1000ms计数
			input[3] = inputcount / 100;
			input[4] = (inputcount % 100) / 10;
			input[5] = inputcount % 10;
			DisplayPassword(input, count);
			if(inputcount  == 0) break;
		}
	}
	for (i = 0; i < 6; ++i)
	{
		input[i] = 10;
	}
	DisplayPassword(input, count);
	P47 = 1;
	msecond = 0;
}

/**************** 密码检查，初始密码为 123456；*******************/
void validate()
{
	u8 i;
	u8 pass[6] = {1,2,3,4,5,6};
	u8 t = inputcount;
	if(count < 6)//密码长度不对，密码错误
	{
		 inputcount++;			
	}
	else
	{
		for(i = 0;i < 6;i++)
		{
			if(input[i] != pass[i])
			{
				 inputcount++;	   //密码错误
				  break;
			}
		}		
	}
	if(t == inputcount)
	{
		for(i = 0;i < 6;i++)
  		input[i] = 1;	   //密码正确，全部显示1
		count = 1;	  //尾数显示1
		inputcount = 0;
		time = 0;
		state = 1;
		DisplayPassword(input, count);
	//	open_success();
		SendState();
		UnlockSuccess();
		return ;
	}
	else
	{
		for(i = 0;i < 6;i++)
		input[i] = 10;
		count = 0;
		DisplayPassword(input, count);
	}
	if(inputcount == 3)//三次错误
	{
		//倒计时
		count_down();
		Alarm("Wrong password for three times\0");//报警
	}
}



/********************** Timer0 1ms中断函数 ************************/
void timer0 (void) interrupt TIMER0_VECTOR
{
	B_1ms = 1;		//1ms标志
	DisplayScan();	//1ms扫描显示一位
}



/*****************************************************
	行列键扫描程序
	使用XY查找4x4键的方法，只能单键，速度快

   Y     P04      P05      P06      P07
          |        |        |        |
X         |        |        |        |
P00 ---- K00 ---- K01 ---- K02 ---- K03 ----
          |        |        |        |
P01 ---- K04 ---- K05 ---- K06 ---- K07 ----
          |        |        |        |
P02 ---- K08 ---- K09 ---- K10 ---- K11 ----
          |        |        |        |
P03 ---- K12 ---- K13 ---- K14 ---- K15 ----
          |        |        |        |
******************************************************/


u8 code T_KeyTable[16] = {0,1,2,0,3,0,0,0,4,0,0,0,0,0,0,0};

void IO_KeyDelay(void)
{
	u8 i;
	i = 60;
	while(--i)	;
}

void IO_KeyScan(void)	//50ms call
{
	u8	j;

	j = IO_KeyState1;	//保存上一次状态

	P0 = 0xf0;	//X低，读Y
	IO_KeyDelay();
	IO_KeyState1 = P0 & 0xf0;

	P0 = 0x0f;	//Y低，读X
	IO_KeyDelay();
	IO_KeyState1 |= (P0 & 0x0f);
	IO_KeyState1 ^= 0xff;	//取反
	
	if(j == IO_KeyState1)	//连续两次读相等
	{
		j = IO_KeyState;
		IO_KeyState = IO_KeyState1;
		if(IO_KeyState != 0)	//有键按下
		{
			F0 = 0;
			if(j == 0)	F0 = 1;	//第一次按下
			else if(j == IO_KeyState)
			{
				if(++IO_KeyHoldCnt >= 20)	//1秒后重键
				{
					IO_KeyHoldCnt = 18;
					F0 = 1;
				}
			}
			if(F0)
			{
				j = T_KeyTable[IO_KeyState >> 4];
				if((j != 0) && (T_KeyTable[IO_KeyState& 0x0f] != 0)) 
					KeyCode = (j - 1) * 4 + T_KeyTable[IO_KeyState & 0x0f] + 16;	//计算键码，17~32
			}
		}
		else	IO_KeyHoldCnt = 0;
	}
	P0 = 0xff;
}
