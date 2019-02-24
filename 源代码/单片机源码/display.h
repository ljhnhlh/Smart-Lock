#ifndef STC_DISPLAY_H
#define STC_DISPLAY_H

#include "STC15Fxxxx.H"
#define DIS_DOT		0x20
#define DIS_BLACK	0x10
#define DIS_		0x11

u8 code t_display[]={						//标准字库
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//位码


sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock

			  
u8 	LED8[8];		//显示缓冲
u8	display_index;	//显示位索引
u8 year,month,day,hour,minute,second;  //年、月、日、时、分、秒变量

void init_time()
{
	year = 19;
	month = 12;
	day = 31;
	hour = 21;
	minute = 55;
	second = 0;
}

/********************** 显示密码函数 ************************/
void DisplayPassword(u8 input[6], u8 count)
{
	u8 i;
	for(i = 0;i < 6;i++)
	{
	    if(input[i]	< 10)
		    LED8[i] = input[i];
		else
		    LED8[i] = DIS_BLACK;
    }
	LED8[6] = DIS_BLACK;
	LED8[7] = count;
}

/********************** 显示温度函数 ************************/
// void DisplayTemperature(u16 j)
// {
// 	LED8[4] = j / 1000;		//显示温度值
// 	LED8[5] = (j % 1000) / 100;
// 	LED8[6] = (j % 100) / 10 + DIS_DOT;
// 	LED8[7] = j % 10;
// 	if(LED8[4] == 0)	LED8[4] = DIS_BLACK;
// }

/**************** 向HC595发送一个字节函数 ******************/
void Send_595(u8 dat)
{		
	u8	i;
	for(i=0; i<8; i++)
	{
		dat <<= 1;
		P_HC595_SER   = CY;
		P_HC595_SRCLK = 1;
		P_HC595_SRCLK = 0;
	}
}

/********************** 显示扫描函数 ************************/
void DisplayScan(void)
{	
	Send_595(~T_COM[display_index]);				//输出位码
	Send_595(t_display[LED8[display_index]]);	//输出段码

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//锁存输出数据
	if(++display_index >= 8)	display_index = 0;	//8位结束回0
}

//========================================================================
// 函数: void  delay_ms(unsigned char ms)
// 描述: 延时函数。
// 参数: ms,要延时的ms数, 这里只支持1~255ms. 自动适应主时钟. 
//========================================================================

// void  delay_ms(u8 ms)
// {
//      u16 i;
// 	 do{
// 	      i = MAIN_Fosc / 13000;
// 		  while(--i)	;   //14T per loop
//      }while(--ms);
// }

/********************** 显示时钟函数 ************************/
void	DisplayRTC(u8 d)
{
	if(d == 0) {
		if(hour >= 10)	LED8[0] = hour / 10;
		else			LED8[0] = DIS_BLACK;
		LED8[1] = hour % 10;
		LED8[2] = DIS_;
		LED8[5] = DIS_;
		LED8[3] = minute / 10;
		LED8[4] = minute % 10;
		LED8[6] = second / 10;
		LED8[7] = second % 10;
	}
	else {
		LED8[0] = 2;
		LED8[1] = 0;
		LED8[2] = year / 10;
		LED8[3] = year % 10;
		LED8[4] = month / 10;
		LED8[5] = month % 10;
		LED8[6] = day / 10;
		LED8[7] = day % 10;
	}
}

/********************** 读RTC函数 ************************/
void	ReadRTC(void)
{
	u8	tmp1[4];
	u8	tmp2[2];
	ReadNbyte(2, tmp1, 4);
	ReadNbyte(7, tmp2, 2);
	second = ((tmp1[0] >> 4) & 0x07) * 10 + (tmp1[0] & 0x0f);
	minute = ((tmp1[1] >> 4) & 0x07) * 10 + (tmp1[1] & 0x0f);
	hour   = ((tmp1[2] >> 4) & 0x03) * 10 + (tmp1[2] & 0x0f);
	day    = ((tmp1[3] >> 4) & 0x0f) * 10 + (tmp1[3] & 0x0f);
	month  = ((tmp2[0] >> 4) & 0x0f) * 10 + (tmp2[0] & 0x0f);
	year   = ((tmp2[1] >> 4) & 0x0f) * 10 + (tmp2[1] & 0x0f);
}

/********************** 写RTC函数 ************************/
void	WriteRTC(void)
{
	u8	tmp1[4];
	u8	tmp2[2];

	tmp1[0] = ((second / 10) << 4) + (second % 10);
	tmp1[1] = ((minute / 10) << 4) + (minute % 10);
	tmp1[2] = ((hour / 10) << 4) + (hour % 10);
	tmp1[3] = ((day / 10) << 4) + (day % 10);
	tmp2[0] = ((month / 10) << 4) + (month % 10);
	tmp2[1] = ((year / 10) << 4) + (year % 10);
	WriteNbyte(2, tmp1, 4);
	WriteNbyte(7, tmp2, 2);
}

#endif


