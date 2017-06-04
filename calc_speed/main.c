#include <reg51.h>
/*******************************************
// T0计数器引脚P3.4
// T1定时器	在T0溢出时被打开：开始计时
********************************************/

#define Firm_Speed 	15	  		// 基准水速
#define MAX_ANGLE		20				// 最大转角 （当前水速 - 基准水速）
#define MIN_ANGLE 	-20				// 最小转角

#define MAX_ALARM		20
#define MIN_ALARM		10

#define Motor_Base	4					// 步进电机单位距离对应（A-B-C-D）多少次		灵敏度

#define N 		1	 							// 多少滴水计一次速度
#define Vol 	0.018						// 一滴水的体积

#define Duan P1						// 数码管段选
#define Wei	 P2						// 数码管位选
#define Motor P0          // 步进电机控制接口定义

sfr T2CON = 0xC8;
sfr T2MOD = 0xC9;
sfr RCAP2L = 0xCA;
sfr RCAP2H = 0xCB;
sfr TL2 = 0xCC;
sfr TH2 = 0xCD;
sbit TR2 = T2CON^2;
sbit ET2 = IE^5;

/*******************/
// 系统参数
unsigned char reset = 0;
unsigned int times = 0;		// 间隔时长

/*******************/
// 报警参数
sbit Alarm = P3^5;
sbit Led = P3^6;
int led_frq = 0;
int led_time = 0;
char led_bit = 0;
char alarm_bit = 0;

/*******************/
// 电机参数
float speed = 0.0;
int angle = 0;
int all = 0;
int move = 0;

unsigned char flag = 0;
unsigned char max = 0;
unsigned char min = 0;

/*******************/
// 函数
void init_timer_counter();
void display(float sp);
void delay(unsigned int c); 
float calc();
void motor(int len);

const unsigned char code number[12] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x79, 0x71};

unsigned char phase_r[4] ={0x08,0x04,0x02,0x01};//正转 电机导通相序 D-C-B-A
unsigned char phase_l[4]={0x01,0x02,0x04,0x08};//反转 电机导通相序 A-B-C-D


void main()
{
	delay(10);
	init_timer_counter();
	Wei = 0;
	
	while(1)
	{

			
		if(flag) {
			motor(angle);
			all += angle;
		}
		flag = 0; 
		display(speed);
		delay(2);
	}
}

void init_timer_counter()		   //初始化定时器计数器
{
	// TIMER0 -> count | TIMER1 -> 8 bit timer
	TMOD = 0x16;		// 0001 0110	  定时器0和1模式寄存器
	T2CON = 0x00;		// 							定时器2状态寄存器
	// T2MOD 感觉不用设置
	TH0 = 256 - N;
	TL0 = 0xff;
	TH1 = (65536-50000)/256;				// 12MHZ -> 50ms=0.05s
	TL1 = (65536-50000)%256;
	
	RCAP2L = TH2 = (65536-25000)/256;
	RCAP2H = TL2 = (65536-25000)%256;
	
	// Set counter T0 input
	// T0 = 1;
	
	EA = 1;					// 打开定时器总中断
	ET0 = 1;				// 允许T0中断
	ET1 = 1;				// 允许T1中断
	ET2 = 1;				// 允许T2中断
	TR0 = 1;				// Open counter	定时器T0运行控制位
}

void motor(int len)
{
	if(len == 0)
		return;
	
	len *= Motor_Base;
	
	if(len > 0) {					// 正转
		unsigned char i;
		for(len;len>0;len--) {
			for(i=0;i<4;i++)
			{
				Motor=phase_r[i];
				display(speed);
			}
		}
	}
	else {						// 反转
		int a = -len;
		unsigned char j;
		for(a;a>0;a--) {
			for(j=0;j<4;j++)
			{
				Motor=phase_l[j];
				display(speed);
			}
		}
	}
	Motor = 0x00;			// 停止
}

float calc()	//计算流量
{
	float sp;
	sp = 3600000/(times*50)*Vol;
	if(sp >= 100)
		return (float)100;
	return sp;
}

void display(float sp)
{
	
	int ge, shi, point;
	int bai = (int)sp / 100;
	if(bai) {
		shi = 10;
		ge = 0;
		point = 11;
	}
	else {
		point = (int)(sp*10) % 10;
		ge = (int)sp % 10;
		shi = (int)(sp/10) % 10;
	}
	
	// point
	Wei = 0x10;
	Duan = number[point];
	delay(2);
	
	
	// ge
	Wei = 0x20;
	Duan = number[ge] | 0x80;		// Add a pointer
	delay(2);

	
	// shi
	Wei = 0x00;
	Duan = number[shi];
	
}

void counter0() interrupt 1		  //计数器0，定时器1
{
	if(!reset) {		// implement 1st time
		TR1 = 1;			// Timer1 start
		reset = 1;
	}
	else {
		// Reset timer1
		TR1 = 0;	 //关闭定时器1
		TH1 = (65536-50000)/256;
		TL1 = (65536-50000)%256;
		TR1 = 1;	 //打开定时器1
		speed = calc();
		
		// 如果太快、慢就打开定时器T2报警
		if(speed > MAX_ALARM)	// 太快
		{
			led_frq = 100;
			TR2 = 1;		// 打开定时器2
		}
		else if(speed < MIN_ALARM)	// 太慢
		{
			led_frq = 200;
			TR2 = 1;
		}
		else	// 速度还可以
		{
			TR2 = 0;
			Led = 0;
			Alarm = 0;
			
			led_time =0;
			led_bit = 0;
			alarm_bit = 0;
			
		}
		move = (int)speed - Firm_Speed;
		if( max )
		{
			if(move > MAX_ANGLE)
				angle = 0;
			else if(move < MIN_ANGLE) {
				angle = MIN_ANGLE - MAX_ANGLE;
				max = 0;
				min = 1;
			}
			else {
				angle = move - MAX_ANGLE;
				max = 0;
			}
		}
		else if( min )
		{
			if(move < MIN_ANGLE)
				angle = 0;
			else if(move > MAX_ANGLE) {
				angle = MAX_ANGLE - MIN_ANGLE;
				max = 1;
				min = 0;
			}
			else {
				angle = move - MIN_ANGLE;
				min = 0;
			}
		}
		else {
			if(move > MAX_ANGLE) {			// Too Fast
				angle = MAX_ANGLE - all;
				max = 1;
				min = 0;
			}
			else if(move < MIN_ANGLE) {	// Too slow
				angle = MIN_ANGLE - all;
				min = 1;
				max = 0;
			}
			else {											// Normal
				angle = move - all;
				max = 0;
				min = 0;
			}
		}
	}
	flag = 1;
	times = 0;
}

void delay(unsigned int c)
{
	unsigned char a,b;
 for (;c>0;c--)
	{
		for (b=10;b>0;b--)
			for (a=130;a>0;a--);  
	}
}

void timer1() interrupt 3
{
	times++;
	TR1 = 0;
	TH1 = (65536-50000)/256;
	TL1 = (65536-50000)%256;
	TR1 = 1;
}

void alarm2() interrupt 4
{
	TR2 = 0;
	alarm_bit = !alarm_bit;
	Alarm = alarm_bit;
	if(led_time == led_frq) {
		led_bit = !led_bit;
		Led = led_bit;
		led_time = 0;
	}	
	led_time++;
	TR2 = 1;
}