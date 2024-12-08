#include "stdint.h"
#define RCC_ADDRESS   0x40023800
#define TIM1_ADDRESS  0x40010000
#define TIM2_ADDRESS  0x40000000
#define GPIOA_ADDRESS 0x40020000
#define GPIOD_ADDRESS 0x40020C00
uint32_t* AHB1 = (uint32_t*)(RCC_ADDRESS + 0x30);
uint32_t* APB1 = (uint32_t*)(RCC_ADDRESS + 0x40);
uint32_t* APB2 = (uint32_t*)(RCC_ADDRESS + 0x44);
#define RCC_TIM1  (*APB2 |= (1 << 0))
#define RCC_TIM2  (*APB1 |= (1 << 0))
#define RCC_GPIOA (*AHB1 |= (1 << 0))
#define RCC_GPIOD (*AHB1 |= (1 << 3))
uint32_t* GPIOA_ODR = (uint32_t*)(GPIOA_ADDRESS + 0x14);
#define HIGH_LEVEL  (*GPIOA_ODR |= (1 << 5))
#define LOW_LEVEL   (*GPIOA_ODR &= ~(1 << 5))
uint32_t* GPIOD_ODR = (uint32_t*)(GPIOD_ADDRESS + 0x14);
#define LED_ON      (*GPIOD_ODR |= (1 << 14))
#define LED_OFF     (*GPIOD_ODR &= ~(1 << 14))
uint32_t status = 0;
uint32_t tick_pre = 0;
uint32_t tick_cur = 0;
uint32_t period = 0;
float distance = 0;
/*************Configuration for the source clock using HSI mode********************/
void Init_RCC()
{
	uint32_t* CR = (uint32_t*)(RCC_ADDRESS + 0x00);
	uint32_t* CFGR = (uint32_t*)(RCC_ADDRESS + 0x08);
	*CR = 0;
	*CFGR = 0;
	*CFGR &= ~(0b11 << 0);
	*CR |= (1 << 0);
	while(!((*CR >> 1) & 1));
}
/**********************************************************************************/

/*************Configuration for the TIMER1 in time base unit***********************/
void Init_TIM1()
{
	RCC_TIM1;
	uint16_t* PSC = (uint16_t*)(TIM1_ADDRESS + 0x28);
	uint16_t* ARR = (uint16_t*)(TIM1_ADDRESS + 0x2C);
	uint16_t* CR1 = (uint16_t*)(TIM1_ADDRESS + 0x00);
	uint16_t* SR = (uint16_t*)(TIM1_ADDRESS + 0x10);
	*PSC = 16 - 1;
	*ARR = 0;
	*ARR = 0xffff - 1;
	*CR1 |= (1 << 0);
	while(!((*SR >> 0) & 1));
	*SR &= ~(1 << 0);
	*CR1 &= ~(1 << 0);
}
void delay_us(uint16_t time)
{
	uint16_t* CR1 = (uint16_t*)(TIM1_ADDRESS + 0x00);
	uint16_t* CNT = (uint16_t*)(TIM1_ADDRESS + 0x24);
	*CNT = 0;
	*CR1 |= (1 << 0);
	while(*CNT < time);
}
void delay_ms(uint16_t time)
{
	for(uint16_t i = 0; i < time; i++)
	{
		delay_us(1000);
	}
}
/**********************************************************************************/

/****************Configuration for the GPIOA(PA5) in Output mode*******************/
void Init_GPIOA()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* OTYPER = (uint32_t*)(GPIOA_ADDRESS + 0x04);
	*MODER &= ~(0b11 << 10);
	*MODER |= (0b01 << 10);
	*OTYPER &= ~(1 << 5);
}
/**********************************************************************************/

/****************Configuration for the GPIOA(PA5) in Output mode*******************/
void Init_GPIOD()
{
	RCC_GPIOD;
	uint32_t* MODER = (uint32_t*)(GPIOD_ADDRESS + 0x00);
	uint32_t* OTYPER = (uint32_t*)(GPIOD_ADDRESS + 0x04);
	*MODER &= ~(0b11 << 28);
	*MODER |= (0b01 << 28);
	*OTYPER &= ~(1 << 14);
}
/**********************************************************************************/

/*************Configuration for the TIMER2 in Input Capture mode*******************/
void Init_TIM2()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* AFRL 	= (uint32_t*)(GPIOA_ADDRESS + 0x20);
	*MODER &= ~(0b11 << 2);
	*MODER |= (0b10 << 2);
	*AFRL |= (1 << 4);

	RCC_TIM2;
	uint16_t* PSC = (uint16_t*)(TIM2_ADDRESS + 0x28);
	uint32_t* ARR = (uint32_t*)(TIM2_ADDRESS + 0x2C);
	uint16_t* CCMR1 = (uint16_t*)(TIM2_ADDRESS + 0x18);
	uint16_t* CCER = (uint16_t*)(TIM2_ADDRESS + 0x20);
	uint16_t* CR1 = (uint16_t*)(TIM2_ADDRESS + 0x00);
	uint16_t* SR = (uint16_t*)(TIM2_ADDRESS + 0x10);
	uint32_t* NVIC_ISER0 = (uint32_t*)(0xE000E100);
	*PSC = 16 - 1;
	*ARR = 0;
	*ARR = 0xffff - 1;
	*CCMR1 |= (0b01 << 8);
	*CCER &= ~((1 << 5)|(1 << 7));
	*CCER |= (1 << 4);
	*NVIC_ISER0 |= (1 << 28);
	*CR1 |= (1 << 0);
	while(!((*SR >> 0) & 1));
	*SR &= ~(1 << 0);
	*CR1 &= ~(1 << 0);
}
void TIMER_Capture()
{
	uint32_t* CCR2 = (uint32_t*)(TIM2_ADDRESS + 0x38);
	uint16_t* CCER = (uint16_t*)(TIM2_ADDRESS + 0x20);
	uint16_t* CR1 = (uint16_t*)(TIM2_ADDRESS + 0x00);
	uint32_t* CNT = (uint32_t*)(TIM2_ADDRESS + 0x24);
	uint16_t* DIER = (uint16_t*)(TIM2_ADDRESS + 0x0C);
	if(status == 0)
	{
		tick_pre = *CCR2;
		status = 1;
		*CCER |= (1 << 5);
		*CCER &= ~(1 << 7);
	}
	else if(status == 1)
	{
		tick_cur = *CCR2;
		*CR1 &= ~(1 << 0);
		*CNT = 0;
		if(tick_pre < tick_cur)
		{
			period = tick_cur - tick_pre;
		}
		else
		{
			period = (0xffff - tick_pre) + tick_cur;
		}
		distance = (float)(period * 0.0340)/2;
		status = 0;
		*CCER &= ~((1 << 5)|(1 << 7));
		*DIER &= ~(1 << 2);
	}
}
void Read_SRF05()
{
	uint16_t* CR1 = (uint16_t*)(TIM2_ADDRESS + 0x00);
	uint16_t* DIER = (uint16_t*)(TIM2_ADDRESS + 0x0C);
	*CR1 |= (1 << 0);
	HIGH_LEVEL;
	delay_us(10);
	LOW_LEVEL;
	*DIER |= (1 << 2);
}
/**********************************************************************************/

/**********************************Main Function***********************************/
int main()
{
	Init_RCC();
	Init_TIM1();
	Init_TIM2();
	Init_GPIOA();
	Init_GPIOD();
	uint32_t* VTOR = (uint32_t*)(0xE000ED08);
	*VTOR = 0x20000000;
	uint32_t* pTIM2 = (uint32_t*)(0x200000B0);
	*pTIM2 = (uint32_t)(TIMER_Capture) | 1;
	while(1)
	{
		Read_SRF05();
		if(distance <= 20)
			LED_ON;
		else
			LED_OFF;
		delay_ms(100);
	}
	return 1;
}
/**********************************************************************************/
