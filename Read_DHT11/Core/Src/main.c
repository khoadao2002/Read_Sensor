/*
 * main.c
 *
 *  Created on: Mar 16, 2024
 *      Author: khoad
 */
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#define RCC_ADDRESS     0x40023800
#define GPIOA_ADDRESS   0x40020000
#define TIMER1_ADDRESS  0x40010000
#define USART2_ADDRESS  0x40004400
uint32_t* AHB1 = (uint32_t*)(RCC_ADDRESS + 0x30);
uint32_t* APB1 = (uint32_t*)(RCC_ADDRESS + 0x40);
uint32_t* APB2 = (uint32_t*)(RCC_ADDRESS + 0x44);
uint32_t* ODR_GPIOA = (uint32_t*)(GPIOA_ADDRESS + 0x14);
#define RCC_GPIOA		(*AHB1 |= (1 << 0))
#define RCC_USART2      (*APB1 |= (1 << 17))
#define RCC_TIMER1      (*APB2 |= (1 << 0))
#define HIGH_LEVEL      (*ODR_GPIOA |= (1 << 7))
#define LOW_LEVEL		(*ODR_GPIOA &= ~(1 << 7))
#define T_H_DATA_SIZE   64
/**************************Configuration for the source clock in HSI mode*********************************/
void Init_Source_Clock()
{
	uint32_t* CFGR = (uint32_t*)(RCC_ADDRESS + 0x08);
	uint32_t* CR = (uint32_t*)(RCC_ADDRESS + 0x00);
	*CFGR &= ~(0b11 << 0);
	*CR |= (1 << 0);
	while(!((*CR >> 1) & 1));
}
/*********************************************************************************************************/

/**************************Configuration for the TIMER1 in time base unit*********************************/
void Init_Delay()
{
	RCC_TIMER1;
	uint16_t* PSC = (uint16_t*)(TIMER1_ADDRESS + 0x28);
	uint16_t* ARR = (uint16_t*)(TIMER1_ADDRESS + 0x2C);
	uint16_t* CR1 = (uint16_t*)(TIMER1_ADDRESS + 0x00);
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
	uint16_t* CR1 = (uint16_t*)(TIMER1_ADDRESS + 0x00);
	uint16_t* CNT = (uint16_t*)(TIMER1_ADDRESS + 0x24);
	*CR1 |= (1 << 0);
	*CNT = 0;
	while(*CNT < time);
	*CR1 &= ~(1 << 0);
}
void delay_ms(uint16_t time)
{
	for(uint16_t i = 0; i < time; i++)
	{
		delay_us(1000);
	}
}
/*********************************************************************************************************/

/******************************Configuration for the USART2 in TX mode************************************/
void Init_USART2()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* AFRL = (uint32_t*)(GPIOA_ADDRESS + 0x20);
	*MODER &= ~(0b11 << 4);
	*MODER |= (0b10 << 4);
	*AFRL |= (7 << 8);

	RCC_USART2;
	uint32_t* CR1 = (uint32_t*)(USART2_ADDRESS + 0x0C);
	uint32_t* CR2 = (uint32_t*)(USART2_ADDRESS + 0x10);
	uint32_t* BBR = (uint32_t*)(USART2_ADDRESS + 0x08);
	*CR1 |= (1 << 13);				//Enable USART
	*CR1 &= ~(1 << 12);				//Set up transmitter 8 bits
	*CR2 &= ~(0b11 << 12);			//Set up Stop bit equal 1
	*BBR |= (8 << 4) | (11 << 0);	//Set up baud rate equal 115.2KBps
	*CR1 |= (1 << 3);				//Enable Transmitter
}
void Transmit_one_byte(uint8_t c)
{
	uint32_t* SR = (uint32_t*)(USART2_ADDRESS + 0x00);
	uint32_t* DR = (uint32_t*)(USART2_ADDRESS + 0x04);
	while(!((*SR >> 7) & 1));
	*DR = c;
	while(!((*SR >> 6) & 1));
}
void Transmit_data(char* data)
{
	for(uint8_t i = 0; data[i] != '\0'; i++)
	{
		Transmit_one_byte(data[i]);
	}
}
/*********************************************************************************************************/

/*************************************Set up for the DHT11 Sensor*****************************************/
void DATA_OUTPUT_MODE()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* OTYPER = (uint32_t*)(GPIOA_ADDRESS + 0x04);
	*MODER &= ~(0b11 << 14);
	*MODER |= (0b01 << 14);
	*OTYPER &= ~(1 << 7);
}
void DATA_INPUT_MODE()
{
	RCC_GPIOA;
	uint32_t* MODER = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	uint32_t* PUPDR = (uint32_t*)(GPIOA_ADDRESS + 0x0C);
	*MODER &= ~(0b11 << 14);
	*PUPDR &= ~(0b11 << 14);
}
void Start_Communicate_to_DHT11()
{
	DATA_OUTPUT_MODE();
	LOW_LEVEL;
	delay_ms(18);
	HIGH_LEVEL;
	delay_us(40);
	DATA_INPUT_MODE();
}
uint8_t Respone_from_DHT11()
{
	uint8_t respone = 0;
	uint32_t* IDR = (uint32_t*)(GPIOA_ADDRESS + 0x10);
	delay_us(40);
	if(!((*IDR >> 7) & 1))
	{
		delay_us(80);
		if(((*IDR >> 7) & 1))
			respone = 1;
		else
			respone = -1;
		while(((*IDR >> 7) & 1));
	}
	return respone;
}
uint8_t Read_Data()
{
	uint32_t* IDR = (uint32_t*)(GPIOA_ADDRESS + 0x10);
	uint8_t data = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		while(!((*IDR >> 7) & 1));
		delay_us(40);
		if(((*IDR >> 7) & 1))
			data |= (1 << (7 - i));
		else if(!((*IDR >> 7) & 1))
			data &= ~(1 << (7 - i));
		while(((*IDR >> 7) & 1));
	}
	return data;
}
void Temperature_and_Humidity()
{
	char* data;
	data = (char*)calloc(T_H_DATA_SIZE, sizeof(char));
	uint8_t res = 0, RH_1 = 0, RH_2 = 0, Temp_1 = 0, Temp_2 = 0, Sum = 0;
	Start_Communicate_to_DHT11();
	res = Respone_from_DHT11();
	if(res == 1)
	{
		RH_1 = Read_Data();
		(void)RH_1;
		RH_2 = Read_Data();
		(void)RH_2;
		Temp_1 = Read_Data();
		(void)Temp_1;
		Temp_2 = Read_Data();
		(void)Temp_2;
		Sum = Read_Data();
		(void)Sum;
		sprintf(data, "Humidity: %d.%d", RH_1, RH_2);
		strcat(data, "%\r\n");
		uint8_t i;
		for(i = 0; data[i] != '\0'; i++);
		sprintf((data + i), "Temperature: %d.%d*C\r\n\r\n", Temp_1, Temp_2);
		Transmit_data(data);
		free(data);
	}
	else
	{
		Transmit_data("Transmission error!!!\r\n");
	}
}
/*********************************************************************************************************/

/******************************************Main Function**************************************************/
int main()
{
	Init_Source_Clock();
	Init_Delay();
	Init_USART2();
	while(1)
	{
		Temperature_and_Humidity();
		delay_ms(2000);
	}
	return 1;
}
/*********************************************************************************************************/
