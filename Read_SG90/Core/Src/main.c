/*
 * main.c
 *
 *  Created on: Jan 17, 2024
 *      Author: khoad
 */
#include "stdint.h"
#define RCC_ADDRESS  0x40023800
#define TIM1_ADDRESS 0x40010000
#define TIM5_ADDRESS 0x40000C00
#define GPIOA_ADDRESS 0x40020000
uint32_t* AHB1 = (uint32_t*)(RCC_ADDRESS + 0x30);
uint32_t* APB1 = (uint32_t*)(RCC_ADDRESS + 0X40);
uint32_t* APB2 = (uint32_t*)(RCC_ADDRESS + 0x44);
#define RCC_TIM5  (*APB1 |= (1 << 3))
#define RCC_GPIOA (*AHB1 |= (1 << 0))
#define RCC_TIM1  (*APB2 |= (1 << 0))
typedef enum
{
	pos_0 = 1000,
	pos_90 = 1500,
	pos_180 = 2000,
}position_t;
/***************Configuration for the source clock using HSI mode***************************/
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
/*******************************************************************************************/

/********************Configuration for the TIM1 in Time base unit***************************/
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
	uint16_t* CNT = (uint16_t*)(TIM1_ADDRESS + 0x24);
	uint16_t* CR1 = (uint16_t*)(TIM1_ADDRESS + 0x00);
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
/*******************************************************************************************/

/********************Configuration for the TIM5 in Output Compare***************************/
void Init_TIM5()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* AFRL = (uint32_t*)(GPIOA_ADDRESS + 0x20);
	*MODER &= ~(0b11 << 4);
	*MODER |= (0b10 << 4);
	*AFRL |= (2 << 8);

	RCC_TIM5;
	uint16_t* PSC = (uint16_t*)(TIM5_ADDRESS + 0x28);
	uint32_t* ARR = (uint32_t*)(TIM5_ADDRESS + 0x2C);
	uint16_t* CCMR2 = (uint16_t*)(TIM5_ADDRESS + 0x1C);
	uint16_t* CCER = (uint16_t*)(TIM5_ADDRESS + 0x20);
	uint16_t* CR1 = (uint16_t*)(TIM5_ADDRESS + 0x00);
	uint16_t* SR = (uint16_t*)(TIM5_ADDRESS + 0x10);
	*PSC = 16 - 1;
	*ARR = 0;
	*ARR = 20000 - 1;
	*CCMR2 &= ~(0b11 << 0);
	*CCMR2 |= (0b110 << 4);
	*CCER |= (1 << 8);
	*CR1 |= (1 << 0);
	while(!((*SR >> 0) & 1));
	*SR &= ~(1 << 0);
}
void Position_SG90(position_t pos)
{
	uint32_t* CCR3 = (uint32_t*)(TIM5_ADDRESS + 0x3C);
	*CCR3 = pos;
}
/*******************************************************************************************/

/****************************************Main Function**************************************/
int main()
{
	Init_RCC();
	Init_TIM5();
	Init_TIM1();
	while(1)
	{
		Position_SG90(pos_0);
		delay_ms(1000);
		Position_SG90(pos_180);
		delay_ms(1000);
	}
	return 1;
}
/*******************************************************************************************/

