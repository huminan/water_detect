#include <reg51.h>
/*******************************************
// T0����������P3.4
// T1��ʱ��	��T0���ʱ���򿪣���ʼ��ʱ
********************************************/

#define Firm_Speed 	15	  		// ��׼ˮ��
#define MAX_ANGLE		20				// ���ת�� ����ǰˮ�� - ��׼ˮ�٣�
#define MIN_ANGLE 	-20				// ��Сת��

#define MAX_ALARM		20
#define MIN_ALARM		10

#define Motor_Base	4					// ���������λ�����Ӧ��A-B-C-D�����ٴ�		������

#define N 		1	 							// ���ٵ�ˮ��һ���ٶ�
#define Vol 	0.018						// һ��ˮ�����

#define Duan P1						// ����ܶ�ѡ
#define Wei	 P2						// �����λѡ
#define Motor P0          // ����������ƽӿڶ���

sfr T2CON = 0xC8;
sfr T2MOD = 0xC9;
sfr RCAP2L = 0xCA;
sfr RCAP2H = 0xCB;
sfr TL2 = 0xCC;
sfr TH2 = 0xCD;
sbit TR2 = T2CON^2;
sbit ET2 = IE^5;

/*******************/
// ϵͳ����
unsigned char reset = 0;
unsigned int times = 0;		// ���ʱ��

/*******************/
// ��������
sbit Alarm = P3^5;
sbit Led = P3^6;
int led_frq = 0;
int led_time = 0;
char led_bit = 0;
char alarm_bit = 0;

/*******************/
// �������
float speed = 0.0;
int angle = 0;
int all = 0;
int move = 0;

unsigned char flag = 0;
unsigned char max = 0;
unsigned char min = 0;

/*******************/
// ����
void init_timer_counter();
void display(float sp);
void delay(unsigned int c); 
float calc();
void motor(int len);

const unsigned char code number[12] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x79, 0x71};

unsigned char phase_r[4] ={0x08,0x04,0x02,0x01};//��ת �����ͨ���� D-C-B-A
unsigned char phase_l[4]={0x01,0x02,0x04,0x08};//��ת �����ͨ���� A-B-C-D


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

void init_timer_counter()		   //��ʼ����ʱ��������
{
	// TIMER0 -> count | TIMER1 -> 8 bit timer
	TMOD = 0x16;		// 0001 0110	  ��ʱ��0��1ģʽ�Ĵ���
	T2CON = 0x00;		// 							��ʱ��2״̬�Ĵ���
	// T2MOD �о���������
	TH0 = 256 - N;
	TL0 = 0xff;
	TH1 = (65536-50000)/256;				// 12MHZ -> 50ms=0.05s
	TL1 = (65536-50000)%256;
	
	RCAP2L = TH2 = (65536-25000)/256;
	RCAP2H = TL2 = (65536-25000)%256;
	
	// Set counter T0 input
	// T0 = 1;
	
	EA = 1;					// �򿪶�ʱ�����ж�
	ET0 = 1;				// ����T0�ж�
	ET1 = 1;				// ����T1�ж�
	ET2 = 1;				// ����T2�ж�
	TR0 = 1;				// Open counter	��ʱ��T0���п���λ
}

void motor(int len)
{
	if(len == 0)
		return;
	
	len *= Motor_Base;
	
	if(len > 0) {					// ��ת
		unsigned char i;
		for(len;len>0;len--) {
			for(i=0;i<4;i++)
			{
				Motor=phase_r[i];
				display(speed);
			}
		}
	}
	else {						// ��ת
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
	Motor = 0x00;			// ֹͣ
}

float calc()	//��������
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

void counter0() interrupt 1		  //������0����ʱ��1
{
	if(!reset) {		// implement 1st time
		TR1 = 1;			// Timer1 start
		reset = 1;
	}
	else {
		// Reset timer1
		TR1 = 0;	 //�رն�ʱ��1
		TH1 = (65536-50000)/256;
		TL1 = (65536-50000)%256;
		TR1 = 1;	 //�򿪶�ʱ��1
		speed = calc();
		
		// ���̫�졢���ʹ򿪶�ʱ��T2����
		if(speed > MAX_ALARM)	// ̫��
		{
			led_frq = 100;
			TR2 = 1;		// �򿪶�ʱ��2
		}
		else if(speed < MIN_ALARM)	// ̫��
		{
			led_frq = 200;
			TR2 = 1;
		}
		else	// �ٶȻ�����
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