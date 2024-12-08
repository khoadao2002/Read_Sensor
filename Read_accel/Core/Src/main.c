/*
 * main.c
 *
 *  Created on: Jan 21, 2024
 *      Author: khoad
 */
#include "stdint.h"
#define RCC_ADDRESS    0x40023800
#define GPIOB_ADDRESS  0x40020400
#define TIMER1_ADDRESS 0x40010000
#define I2C1_ADDRESS   0x40005400
#define GPIOD_ADDRESS  0x40020C00
uint32_t* ODR = (uint32_t*)(GPIOD_ADDRESS + 0x14);
#define LED_RED_ON     (*ODR |= (1 << 12))
#define LED_RED_OFF    (*ODR &= ~(1 << 12))
#define LED_BLUE_ON    (*ODR |= (1 << 15))
#define LED_BLUE_OFF   (*ODR &= ~(1 << 15))
uint8_t WHO_AM_I = 0;
int16_t buffer[3] = {0};
float AX = 0;
float AY = 0;
float AZ = 0;
/******************Configuration for the source clock using HSI mode*****************************/
uint32_t* AHB1 = (uint32_t*)(RCC_ADDRESS + 0x30);
uint32_t* APB1 = (uint32_t*)(RCC_ADDRESS + 0x40);
uint32_t* APB2 = (uint32_t*)(RCC_ADDRESS + 0x44);
#define RCC_TIM1       (*APB2 |= (1 << 0))
#define RCC_GPIOB      (*AHB1 |= (1 << 1))
#define RCC_I2C1       (*APB1 |= (1 << 21))
#define RCC_GPIOD      (*AHB1 |= (1 << 3))

void Init_RCC()
{
	//Set up the source clock equal 16MHZ
	uint32_t* CR = (uint32_t*)(RCC_ADDRESS + 0x00);
	uint32_t* CFGR = (uint32_t*)(RCC_ADDRESS + 0x08);
	*CR = 0;
	*CFGR = 0;
	*CFGR &= ~(0b11 << 0);
	*CR |= (1 << 0);
	while(!((*CR >> 1) & 1));
}
/************************************************************************************************/

/******************Configuration for the TIMER1 using time base unit*****************************/
void Init_TIM1()
{
	RCC_TIM1;
	uint16_t* CR1 = (uint16_t*)(TIMER1_ADDRESS + 0x00);
	uint16_t* PSC = (uint16_t*)(TIMER1_ADDRESS + 0x28);
	uint16_t* ARR = (uint16_t*)(TIMER1_ADDRESS + 0x2C);
	uint16_t* SR = (uint16_t*)(TIMER1_ADDRESS + 0x10);
	*PSC = 16 - 1;
	*ARR = 0xffff - 1;
	*CR1 |= (1 << 0);
	while(!((*SR >> 0) & 1));
	*SR &= ~(1 << 0);
	*CR1 &= ~(1 << 0);
}
void delay_us(uint16_t time)
{
	uint16_t* CNT = (uint16_t*)(TIMER1_ADDRESS + 0x24);
	uint16_t* CR1 = (uint16_t*)(TIMER1_ADDRESS + 0x00);
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
/************************************************************************************************/

/************************************Configuration for I2C***************************************/
void Init_I2C1()
{
	RCC_GPIOB;
	uint32_t* MODER = (uint32_t*)(GPIOB_ADDRESS + 0x00);
	uint32_t* AFRL = (uint32_t*)(GPIOB_ADDRESS + 0x20);
	uint32_t* AFRH = (uint32_t*)(GPIOB_ADDRESS + 0x24);
	uint32_t* PUPDR = (uint32_t*)(GPIOB_ADDRESS + 0x0C);
	uint32_t* OTYPER = (uint32_t*)(GPIOB_ADDRESS + 0x04);
	*MODER &= ~((0b11 << 14)|(0b11 << 16));
	*MODER |= (0b10 << 14)|(0b10 << 16);
	*AFRL |= (4 << 28);
	*AFRH |= (4 << 0);
	*OTYPER |= (1 << 7)|(1 << 8);
	*PUPDR |= (0b01 << 14)|(0b01 << 16);

	RCC_I2C1;
	uint16_t* CR2 = (uint16_t*)(I2C1_ADDRESS + 0x04);
	uint16_t* CCR = (uint16_t*)(I2C1_ADDRESS + 0x1C);
	uint16_t* CR1 = (uint16_t*)(I2C1_ADDRESS + 0x00);
	uint16_t* TRISE = (uint16_t*)(I2C1_ADDRESS + 0x20);
	*CR1 |= (1 << 15);
	*CR1 &= ~(1 << 15);
	*CR2 |= (16 << 0);
	*CCR |= 40;
	*TRISE |= 17;
	*CR1 |= (1 << 0);
}
uint8_t Read_I2C(uint8_t address_reg)
{
	uint16_t* CR1 = (uint16_t*)(I2C1_ADDRESS + 0x00);
	uint16_t* SR1 = (uint16_t*)(I2C1_ADDRESS + 0x14);
	uint16_t* SR2 = (uint16_t*)(I2C1_ADDRESS + 0x18);
	uint16_t* DR = (uint16_t*)(I2C1_ADDRESS + 0x10);
	uint8_t address_slave = (0b1101000 << 1);

	while(((*SR2 >> 1) & 1));
	*CR1 |= (1 << 8);
	while(!((*SR1 >> 0) & 1));
	uint16_t read_SR = *SR1;
	(void)read_SR;
	*DR = (address_slave | 0);
	while(!((*SR1 >> 1)));
	read_SR = *SR1;
	read_SR = *SR2;
	while(!((*SR1 >> 7) & 1));
	*DR = address_reg;
	while(!((*SR1 >> 7) & 1));
	while(!((*SR1 >> 2) & 1));

	*CR1 |= (1 << 8);
	while(!((*SR1 >> 0) & 1));
	read_SR = *SR1;
	*DR = (address_slave | 1);
	while(!((*SR1 >> 1) & 1));
	read_SR = *SR1;
	read_SR = *SR2;
	while(!((*SR1 >> 6) & 1));
	uint8_t data = *DR;
	*CR1 |= (1 << 9);
	return data;
}
void Read_Accelerometer(int16_t* buffer)
{
	uint8_t index = 0;
	for(uint8_t i = 0x3B; i <= 0x3F; i+=2)
	{
		buffer[index++] = (int16_t)((Read_I2C(i) << 8) | (Read_I2C(i + 1) << 0));
	}
	AX = (float)(buffer[0]/16384.0);
	AY = (float)(buffer[1]/16384.0);
	AZ = (float)(buffer[2]/16384.0);
}
void Write_I2C(uint8_t address_reg, uint8_t data)
{
	uint16_t* CR1 = (uint16_t*)(I2C1_ADDRESS + 0x00);
	uint16_t* SR1 = (uint16_t*)(I2C1_ADDRESS + 0x14);
	uint16_t* SR2 = (uint16_t*)(I2C1_ADDRESS + 0x18);
	uint16_t* DR = (uint16_t*)(I2C1_ADDRESS + 0x10);
	uint8_t address_slave = (0b1101000 << 1);

	while(((*SR2 >> 1) & 1));
	*CR1 |= (1 << 8);
	while(!((*SR1 >> 0) & 1));
	uint8_t read_SR = *SR1;
	(void)read_SR;
	*DR = (address_slave | 0);
	while(!((*SR1 >> 1) & 1));
	read_SR = *SR1;
	read_SR = *SR2;
	while(!((*SR1 >> 7) & 1));
	*DR = address_reg;
	while(!((*SR1 >> 7) & 1));
	*DR = data;
	while(!((*SR1 >> 7) & 1));
	while(!((*SR1 >> 2) & 1));
	*CR1 |= (1 << 9);
}
/************************************************************************************************/

/************************Configuration for the GPIOD in Output mode******************************/
void Init_GPIOD()
{
	RCC_GPIOD;
	uint32_t* MODER = (uint32_t*)(GPIOD_ADDRESS + 0x00);
	uint32_t* OTYPER = (uint32_t*)(GPIOD_ADDRESS + 0x04);
	*MODER &= ~((0b11 << 24)|(0b11 << 30));
	*MODER |= (0b01 << 24)|(0b01 << 30);
	*OTYPER &= ~((1 << 12)|(1 << 15));
}
/************************************************************************************************/

/****************************************Main Function*******************************************/
int main()
{
	Init_RCC();
	Init_I2C1();
	Init_TIM1();
	Init_GPIOD();
	WHO_AM_I = Read_I2C(0x75);
	Write_I2C(0x1C, 0x00);  //Configuration for Accelerometer
	while(1)
	{
		Read_Accelerometer(buffer);
		if(AY < -0.7 || AY > 0.7)
		{
			LED_RED_ON;
			LED_BLUE_OFF;
		}
		else if(AZ > 0.7 || AZ < -0.7)
		{
			LED_BLUE_ON;
			LED_RED_OFF;
		}
		else
		{
			LED_BLUE_OFF;
			LED_RED_OFF;
		}

	}
	return 1;
}
/************************************************************************************************/


