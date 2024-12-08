/*
 * main.c
 *
 *  Created on: Nov 8, 2023
 *      Author: khoad
 */
#include "main.h"
#include "math.h"
uint32_t* GPIOE_ODR = (uint32_t*)(0x40021000 + 0x14);
#define SENSOR_INACTIVE *GPIOE_ODR |= (1 << 3)
#define SENSOR_ACTIVE *GPIOE_ODR &= ~(1 << 3)
int16_t value[3] = {0};
int8_t GX = 0;
int8_t GY = 0;
int8_t GZ = 0;
//--------------------------------Configuration System Tick----------------------------------------
unsigned int tick = 0;
void Init_Systick()
{
	unsigned int* SYST_CSR = (unsigned int*)0xE000E010;
	unsigned int* SYST_RVR = (unsigned int*)0xE000E014;
	*SYST_RVR = 16000/1 - 1;
	*SYST_CSR |= (1 << 0)|(1 << 1);
}
void SysTick_Handler_Custom()
{
	tick++;
	unsigned int* SYST_CSR = (unsigned int*)0xE000E010;
	*SYST_CSR &= ~(1 << 16);
}
void delay_ms(unsigned int time)
{
	tick = 0;
	while(tick < time);
}
//---------------------------------------------------------------------------------------------------

//--------------------------------Configuration SPI--------------------------------------------------
void Init_SPI()
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t* GPIOA_MODER = (uint32_t*)(0x40020000 + 0x00);
	uint32_t* GPIOA_AFRL = (uint32_t*)(0x40020000 + 0x20);
	*GPIOA_MODER |= (0b10 << 10)|(0b10 << 12)|(0b10 << 14);
	*GPIOA_AFRL |= (5 << 20)|(5 << 24)|(5 << 28);
	__HAL_RCC_GPIOE_CLK_ENABLE();
	uint32_t* GPIOE_MODER = (uint32_t*)(0x40021000 + 0x00);
	*GPIOE_MODER |= (0b01 << 6);

	//Set up for SPI
	__HAL_RCC_SPI1_CLK_ENABLE();
	uint32_t* CR1 = (uint32_t*)(0x40013000);
	*CR1 |= (0b011 << 3);
	*CR1 |= (1 << 8)|(1 << 9)|(1 << 6)|(1 << 2);
}
//----------------------------------------------------------------------------------------------

//-------------------------------Read One Byte Data---------------------------------------------
uint8_t Read_data(uint8_t data)
{
	SENSOR_ACTIVE;
	uint32_t* SR = (uint32_t*)(0x40013000 + 0x08);
	uint32_t* DR = (uint32_t*)(0x40013000 + 0x0C);
	while(((*SR >> 1) & 1) != 1);

	*DR = data | (1 << 7);
	while(((*SR >> 1) & 1) == 1);
	while(((*SR >> 7) & 1) == 1);
	while(((*SR >> 0) & 1) != 1);
	uint8_t spam_data = (uint8_t)*DR;
	(void)spam_data;

	while(((*SR >> 1) & 1) != 1);
	*DR = 0xFF;
	while(((*SR >> 1) & 1) == 1);
	while(((*SR >> 7) & 1) == 1);
	while(((*SR >> 0) & 1) != 1);
	uint8_t real_data = *DR;
	SENSOR_INACTIVE;
	return real_data;
}
//-----------------------------------------------------------------------------------------------

//---------------------------Write One Byte Data-------------------------------------------------
void write_data(uint8_t address, uint8_t data)
{
	SENSOR_ACTIVE;
	uint32_t* SR = (uint32_t*)(0x40013000 + 0x08);
	uint32_t* DR = (uint32_t*)(0x40013000 + 0x0C);
	while(((*SR >> 1) & 1) != 1);

	*DR = address;
	while(((*SR >> 1) & 1) == 1);
	while(((*SR >> 7) & 1) == 1);
	while(((*SR >> 0) & 1) != 1);
	uint8_t spam_data = (uint8_t)*DR;
	(void)spam_data;

	while(((*SR >> 1) & 1) != 1);
	*DR = data;
	while(((*SR >> 1) & 1) == 1);
	while(((*SR >> 7) & 1) == 1);
	while(((*SR >> 0) & 1) != 1);
	spam_data = *DR;
	SENSOR_INACTIVE;
}
//------------------------------------------------------------------------------------------------

//------------------------------Read Multiple Byte Data-------------------------------------------
void Read_Multi_Byte_Data(uint8_t reg, char* data, uint8_t size)
{
	SENSOR_ACTIVE;
	uint32_t* SR = (uint32_t*)(0x40013000 + 0x08);
	uint32_t* DR = (uint32_t*)(0x40013000 + 0x0C);
	while(((*SR >> 1) & 1) != 1);

	*DR = reg | (1 << 7) | (1 << 6);
	while(((*SR >> 1) & 1) == 1);
	while(((*SR >> 7) & 1) == 1);
	while(((*SR >> 0) & 1) != 1);
	uint8_t spam_data = (uint8_t)*DR;
	(void)spam_data;
	for(int i = 0; i < size; i++)
	{
		while(((*SR >> 1) & 1) != 1);
		*DR = 0xFF;
		while(((*SR >> 1) & 1) == 1);
		while(((*SR >> 7) & 1) == 1);
		while(((*SR >> 0) & 1) != 1);
		data[i] = *DR;
	}
	SENSOR_INACTIVE;
}
//---------------------------------------------------------------------------------------------------

//------------------------------------Remove Vector Table--------------------------------------------
void Remove_VTTB()
{
	unsigned int* VTOR = (unsigned int*)0xE000ED08;
	*VTOR = 0x20000000;
}
//----------------------------------------------------------------------------------------------------

//-------------------------------Main Function--------------------------------------------------------
int main()
{
	Init_Systick();
	Init_SPI();
	Remove_VTTB();
	unsigned int* address_Systick = (unsigned int*)0x2000003C;
	*address_Systick = (unsigned int)SysTick_Handler_Custom | 1;
	write_data(0x20,0b000001111);
	while(1)
	{
		uint8_t status = Read_data(0x27);
		if(((status >> 3) & 1) == 1)
		{
			Read_Multi_Byte_Data(0x28, (char*)value, 6);
			GX = value[0] * 0.00875;
			GY = value[1] * 0.00875;
			GZ = value[2] * 0.00875;
		}
		delay_ms(100);
	}
	return 1;
}
//------------------------------------------------------------------------------------------------------
