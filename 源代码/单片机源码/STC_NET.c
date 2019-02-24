#include "STC_NET.h"
#define UART1_BUF_LENGTH 32
u8 RX1_Cnt;                            //接收计数
bit B_TX1_Busy;                        //发送忙标志
u8 idata RX1_Buffer[UART1_BUF_LENGTH]; //接收缓冲
u8 RX1_Buffer_Temp[UART1_BUF_LENGTH]; //接收缓冲的临时变量
//========================================================================
// 函数: void PrintString1(u8 *puts)
// 描述: 串口1发送字符串函数。
// 参数: puts:  字符串指针.
// 返回: none.
// 版本: VER1.0
// 日期: 2014-11-28
// 备注:
//========================================================================
void PrintString1(u8 *puts) //发送一个字符串
{
    for (; *puts != '\0'; puts++) //遇到停止符0结束
    {
        SBUF = *puts;
        B_TX1_Busy = 1;
        while (B_TX1_Busy);
    }
}
//========================================================================
// 函数: void   UART1_config()
// 描述: UART1初始化函数。
// 参数: void
// 返回: none.
// 版本: VER1.0
// 日期: 2014-11-28
// 备注:
//========================================================================
void UART1_config() // 选择波特率
{
    /*********** 波特率使用定时器1 *****************/
    //115200bps@11.0592MHz
    SCON = 0x50;  //8位数据,可变波特率
    AUXR |= 0x40; //定时器1时钟为Fosc,即1T
    AUXR &= 0xFE; //串口1选择定时器1为波特率发生器
    TMOD &= 0x0F; //设定定时器1为16位自动重装方式
    TL1 = 0xE8;   //设定定时初值
    TH1 = 0xFF;   //设定定时初值
    ET1 = 0;      //禁止定时器1中断
    TR1 = 1;      //启动定时器1
    /*************************************************/
    //  PS  = 1;    //高优先级中断
    ES = 1;  //允许中断
    REN = 1; //允许接收
    P_SW1 &= 0x3f;
    P_SW1 |= 0x80; //UART1 switch to, 0x00: P3.0 P3.1, 0x40: P3.6 P3.7, 0x80: P1.6 P1.7 (必须使用内部时钟)
                   //  PCON2 |=  (1<<4);   //内部短路RXD与TXD, 做中继, ENABLE,DISABLE

    B_TX1_Busy = 0;
    RX1_Cnt = 0;
}
//========================================================================
// 函数: void UART1_int (void) interrupt UART1_VECTOR
// 描述: UART1中断函数。
// 参数: nine.
// 返回: none.
// 备注:
//========================================================================
void UART1_int(void) interrupt 4
{
    if (RI)
    {
        RI = 0;
        RX1_Buffer[RX1_Cnt] = SBUF;
        if (RX1_Buffer[RX1_Cnt] == '\n'){
            RX1_Buffer[RX1_Cnt] = '\0';
            RX1_Cnt = 0;
        }
        else if(++RX1_Cnt >= UART1_BUF_LENGTH){
            RX1_Cnt = 0;
        }
    }

    if (TI)
    {
        TI = 0;
        B_TX1_Busy = 0;
    }
}

//========================================================================
// 函数: void  delay_ms(u16 ms)
// 描述: 延时函数。
// 参数: ms,要延时的ms数, 这里只支持1~65532ms. 自动适应主时钟.
// 返回: none.
// 备注:
//========================================================================
void delay_ms(u16 ms)
{
    u16 i;
    do
    {
        i = MAIN_Fosc / 13000;
        while (--i)
            ; //14T per loop
    } while (--ms);
}

int StringLen(u8* String){
    int i;
    for(i = 0;*(String+i)!='\0';i++);
    return i;
}
//=========================================================================
//函数：bool BuffCMP(u8* cmp)
//参数：比对缓冲区的字符串cmp
//返回：布尔值
bit BuffCMP(u8* cmp)
{
    u8 i;
    strncpy(RX1_Buffer_Temp,RX1_Buffer,32);
    for (i = 0; *cmp != '\0' ; i++, cmp++)
    {
        if(*cmp != RX1_Buffer_Temp[i]){
            return 0;
        }
    }
    //清除接收区缓存
    for(i = 0;i<32;i++){
        RX1_Buffer[i]='\0';
    }
    return 1;
}

bit ATConnect()
{
    do{
        PrintString1("AT\r\n");
        delay_ms(500);
    }
    while(!BuffCMP("OK"));
    return 1;
}

bit ATMode()
{
    do{
        PrintString1("AT+CWMODE_CUR=1\r\n");
        delay_ms(500);
    }
    while(!BuffCMP("OK"));
    return 1;
}

bit ATWifi(u8* username,u8* password)
{
    u8 xdata temp[60];
    sprintf(temp,"AT+CWJAP=\"%s\",\"%s\"\r\n",username,password);
    do{
        PrintString1(temp);
        delay_ms(5000);
    }
    while(!BuffCMP("OK"));
    return 1;
}

//连接TCP服务器
bit TCPConnect(u8* address,u8* port)
{
    u8 xdata temp[50];
    sprintf(temp,"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",address,port);
    do{
        PrintString1(temp);
        delay_ms(2000);
    }
    while(!BuffCMP("OK"));
    return 1;
}

//发送一个字符串
bit TCPSend(u8* message){
    int len;
    u8 xdata temp[24];
    len = StringLen(message);
    sprintf(temp,"AT+CIPSENDEX=%d\r\n\0",len);
    PrintString1(temp);
    do{
        delay_ms(50);
    }
    while(!BuffCMP(">"));
    PrintString1(message);
    do{
        delay_ms(50);
    }
    while(!BuffCMP("SEND OK"));
    return 1;
}