#ifndef STC_RTC_H
#define STC_RTC_H

#define SLAW	0xA2
#define SLAR	0xA3

sbit	SDA	= P1^1;	//定义SDA  PIN5
sbit	SCL	= P1^0;	//定义SCL  PIN6

void	ReadNbyte( u8 addr, u8 *p, u8 number);

/****************************/
void	I2C_Delay(void)	//for normal MCS51,	delay (2 * dly + 4) T, for STC12Cxxxx delay (4 * dly + 10) T
{
	u8	dly;
	dly = MAIN_Fosc / 2000000UL;		//按2us计算
	while(--dly)	;
}

/****************************/
void I2C_Start(void)	//start the I2C, SDA High-to-low when SCL is high
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
 	SDA = 0;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}		


void I2C_Stop(void)		//STOP the I2C, SDA Low-to-high when SCL is high
{
	SDA = 0;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SDA = 1;
	I2C_Delay();
}

void S_ACK(void)		//Send ACK (LOW)
{
	SDA = 0;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}

void S_NoACK(void)		//Send No ACK (High)
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}
		
void I2C_Check_ACK(void)	//Check ACK, If F0=0, then right, if F0=1, then error
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	F0  = SDA;
	SCL = 0;
	I2C_Delay();
}

/****************************/
u8 I2C_ReadAbyte(void)		//read A byte from I2C
{
	u8 i,dat;
	i = 8;
	SDA = 1;
	do
	{
		SCL = 1;
		I2C_Delay();
		dat <<= 1;
		if(SDA)		dat++;
		SCL  = 0;
		I2C_Delay();
	}
	while(--i);
	return(dat);
}

/****************************/
void I2C_WriteAbyte(u8 dat)		//write a byte to I2C
{
	u8 i;
	i = 8;
	do
	{
		if(dat & 0x80)	SDA = 1;
		else			SDA	= 0;
		dat <<= 1;
		I2C_Delay();
		SCL = 1;
		I2C_Delay();
		SCL = 0;
		I2C_Delay();
	}
	while(--i);
}

/****************************/
void WriteNbyte(u8 addr, u8 *p, u8 number)			/*	WordAddress,First Data Address,Byte lenth	*/
                         									//F0=0,right, F0=1,error
{
	I2C_Start();
	I2C_WriteAbyte(SLAW);
	I2C_Check_ACK();
	if(!F0)
	{
		I2C_WriteAbyte(addr);
		I2C_Check_ACK();
		if(!F0)
		{
			do
			{
				I2C_WriteAbyte(*p);		p++;
				I2C_Check_ACK();
				if(F0)	break;
			}
			while(--number);
		}
	}
	I2C_Stop();
}


/****************************/
void ReadNbyte(u8 addr, u8 *p, u8 number)		/*	WordAddress,First Data Address,Byte lenth	*/
{
	I2C_Start();
	I2C_WriteAbyte(SLAW);
	I2C_Check_ACK();
	if(!F0)
	{
		I2C_WriteAbyte(addr);
		I2C_Check_ACK();
		if(!F0)
		{
			I2C_Start();
			I2C_WriteAbyte(SLAR);
			I2C_Check_ACK();
			if(!F0)
			{
				do
				{
					*p = I2C_ReadAbyte();	p++;
					if(number != 1)		S_ACK();	//send ACK
				}
				while(--number);
				S_NoACK();			//send no ACK
			}
		}
	}
	I2C_Stop();
}

#endif