/*
 * main.c
 *
 *  Created on: Mar 30, 2024
 *      Author: khoad
 */
#include "stdint.h"
#include "string.h"
#define RCC_ADDRESS		0x40021000
#define SPI1_ADDRESS	0x40013000
#define AFIO_ADDRESS	0x40010000
#define ADC1_ADDRESS	0x40012400
#define GPIOA_ADDRESS	0x40010800
#define TIMER1_ADDRESS	0x40012C00
#define GPIOB_ADDRESS	0x40010C00
uint32_t* APB2ENR = (uint32_t*)(RCC_ADDRESS + 0x18);
uint32_t* ODR_GPIOA = (uint32_t*)(GPIOA_ADDRESS + 0x0C);
#define SLAVE_ACTIVE	(*ODR_GPIOA &= ~(1 << 4))
#define SLAVE_INACTIVE	(*ODR_GPIOA |= (1 << 4))
#define CE_ACTIVE		(*ODR_GPIOA |= (1 << 3))
#define CE_INACTIVE		(*ODR_GPIOA &= ~(1 << 3))
#define RCC_SPI1		(*APB2ENR |= (1 << 12))
#define RCC_TIMER1		(*APB2ENR |= (1 << 11))
#define RCC_AFIO		(*APB2ENR |= (1 << 0))
#define RCC_ADC1		(*APB2ENR |= (1 << 9))
#define RCC_GPIOA 		(*APB2ENR |= (1 << 2))
#define RCC_GPIOB		(*APB2ENR |= (1 << 3))
uint16_t Joystick_X = 0;
uint16_t Joystick_Y = 0;
uint8_t Address_TX[] = {0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
uint8_t TX_Data[32] = {0};
/*******************************Configuration for the source clock in HSI mode*********************************/
void Init_Source_Clock()
{
	uint32_t* CFGR = (uint32_t*)(RCC_ADDRESS + 0x04);
	uint32_t* CR = (uint32_t*)(RCC_ADDRESS + 0x00);
	*CFGR &= ~(0b11 << 0);
	*CR |= (1 << 0);
	while(!((*CR >> 1) & 1));
}
/**************************************************************************************************************/

/*******************************Configuration for the TIMER1 in time base unit*********************************/
void Init_Delay()
{
	RCC_TIMER1;
	uint16_t* PSC = (uint16_t*)(TIMER1_ADDRESS + 0x28);
	uint16_t* ARR = (uint16_t*)(TIMER1_ADDRESS + 0x2C);
	uint16_t* CR1 = (uint16_t*)(TIMER1_ADDRESS + 0x00);
	uint16_t* SR = (uint16_t*)(TIMER1_ADDRESS + 0x10);
	*PSC = 8 - 1;
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
/**************************************************************************************************************/

/********************************Configuration for the SPI1 in Master mode*************************************/
void Init_SPI1()
{
	RCC_AFIO;
	RCC_GPIOA;
	uint32_t* AFIO_MAPS = (uint32_t*)(AFIO_ADDRESS + 0x04);
	uint32_t* CRL = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	*AFIO_MAPS &= ~(1 << 0);
	*CRL &= ~(0xfffff << 12);
	*CRL |= (0b11 << 12) | (0b11 << 16) | (0b11 << 20) | (0b11 << 28);
	*CRL |= (0b01 << 26);
	*CRL &= ~((0b11 << 14) | (0b11 << 18));
	*CRL |= (0b10 << 22) | (0b10 << 30);

	RCC_SPI1;
	SLAVE_INACTIVE;
	uint16_t* CR1 = (uint16_t*)(SPI1_ADDRESS + 0x00);
	*CR1 |= (0b010 << 3);
	*CR1 &= ~((0b11 << 0) | (1 << 11) | (1 << 7));
	*CR1 |= (1 << 8) | (1 << 9) | (1 << 2) | (1 << 6);
}
void SPI_Transmitter(uint8_t* data, uint8_t size)
{
	uint16_t* SR = (uint16_t*)(SPI1_ADDRESS + 0x08);
	uint16_t* DR = (uint16_t*)(SPI1_ADDRESS + 0x0C);
	while(size--)
	{
		while(!((*SR >> 1) & 1));
		*DR = *data++;
		while(!((*SR >> 0) & 1));
		uint8_t read_DR = *DR;
		(void)read_DR;
	}
	while(!((*SR >> 1) & 1));
	while(((*SR >> 7) & 1));
}
void SPI_Receiver(uint8_t* data, uint8_t size)
{
	uint16_t* SR = (uint16_t*)(SPI1_ADDRESS + 0x08);
	uint16_t* DR = (uint16_t*)(SPI1_ADDRESS + 0x0C);
	while(size--)
	{
		while(!((*SR >> 1) & 1));
		*DR = 0xff;
		while(!((*SR >> 0) & 1));
		*data++ = *DR;
	}
}
void NRF24L01_Write(uint8_t address_reg, uint8_t data)
{
	SLAVE_ACTIVE;
	uint8_t buffer[2] = {0};
	buffer[0] = address_reg | (1 << 5);
	buffer[1] = data;
	SPI_Transmitter(buffer,  2);
	SLAVE_INACTIVE;
}
void NRF24L01_Multi_Write(uint8_t address_reg, uint8_t* data, uint8_t size)
{
	SLAVE_ACTIVE;
	uint8_t buffer = address_reg | (1 << 5);
	SPI_Transmitter(&buffer, 1);
	SPI_Transmitter(data, size);
	SLAVE_INACTIVE;
}
uint8_t NRF24L01_Read(uint8_t address_reg)
{
	uint8_t data = 0;
	SLAVE_ACTIVE;
	SPI_Transmitter(&address_reg, 1);
	SPI_Receiver(&data, 1);
	SLAVE_INACTIVE;
	return data;
}
void NRF24L01_Multi_Read(uint8_t address_reg, uint8_t* data, uint8_t size)
{
	SLAVE_ACTIVE;
	SPI_Transmitter(&address_reg, 1);
	SPI_Receiver(data, size);
	SLAVE_INACTIVE;
}
void NRF24L01_Send_cmd(uint8_t cmd)
{
	SLAVE_ACTIVE;
	SPI_Transmitter(&cmd, 1);
	SLAVE_INACTIVE;
}
/************************************************************************************************************/

/***************************Configuration for the NRF24L01 in Transmission mode******************************/
void Init_NRF24L01()
{
	CE_INACTIVE;
	SLAVE_INACTIVE;
	NRF24L01_Write(0x00, 0x00);
	NRF24L01_Write(0x01, 0x00);
	NRF24L01_Write(0x02, 0x00);
	NRF24L01_Write(0x03, 0x03);
	NRF24L01_Write(0x04, 0x00);
	NRF24L01_Write(0x05, 0x00);
	NRF24L01_Write(0x06, 0x0E);
	CE_ACTIVE;
	SLAVE_ACTIVE;
}
void NRF24L01_TX_mode(uint8_t* address_reg, uint8_t channel)
{
	CE_INACTIVE;
	SLAVE_INACTIVE;
	NRF24L01_Write(0x05, channel);
	NRF24L01_Multi_Write(0x10, address_reg, 5);
	uint8_t config = NRF24L01_Read(0x00);
	config |= (1 << 1);
	NRF24L01_Write(0x00, config);
	CE_ACTIVE;
	SLAVE_ACTIVE;
}
uint8_t NRF24L01_Transmit(uint8_t* data)
{
	uint8_t cmdtosend = 0;
	SLAVE_ACTIVE;
	cmdtosend = 0xA0;
	SPI_Transmitter(&cmdtosend, 1);
	SPI_Transmitter(data, 32);
	SLAVE_INACTIVE;
	delay_ms(1);
	uint8_t fifostatus = NRF24L01_Read(0x17);
	if(((fifostatus >> 4) & 1) && (!((fifostatus >> 3) & 1)))
	{
		cmdtosend = 0xE1;
		NRF24L01_Send_cmd(cmdtosend);
		return 1;
	}
	return 0;
}
/**************************************************************************************************************/

/**************************************Configuration for the ADC1**********************************************/
void Init_ADC1()
{
	RCC_GPIOA;
	RCC_AFIO;
	uint32_t* AFIO_MAPS = (uint32_t*)(AFIO_ADDRESS + 0x04);
	uint32_t* CRL = (uint32_t*)(GPIOA_ADDRESS + 0x00);
	*AFIO_MAPS |= (1 << 17);
	*CRL &= ~((0xf << 4) | (0xf << 8));

	RCC_ADC1;
	uint32_t* CR1 = (uint32_t*)(ADC1_ADDRESS + 0x04);
	uint32_t* CR2 = (uint32_t*)(ADC1_ADDRESS + 0x08);
	uint32_t* JSQR = (uint32_t*)(ADC1_ADDRESS + 0x38);
	*JSQR |= (0b01 << 20);
	*JSQR |= (1 << 15) | (2 << 10);
	*CR2 |= (1 << 15) | (0b111 << 12);
	*CR2 &= ~(1 << 11);
	*CR1 |= (1 << 8);
	*CR2 |= (1 << 0) | (1 << 1);
}
void Read_ADC(uint16_t* Data_X, uint16_t* Data_Y)
{
	uint32_t* CR2 = (uint32_t*)(ADC1_ADDRESS + 0x08);
	uint32_t* SR = (uint32_t*)(ADC1_ADDRESS + 0x00);
	uint32_t* JDR1 = (uint32_t*)(ADC1_ADDRESS + 0x3C);
	uint32_t* JDR2 = (uint32_t*)(ADC1_ADDRESS + 0x40);
	*CR2 |= (1 << 21);
	while(!((*SR >> 2) & 1));
	*SR &= ~(1 << 2);
	*Data_X = *JDR1;
	*Data_Y = *JDR2;
}
/**************************************************************************************************************/

/*************************************************Main Function************************************************/
int main()
{
	Init_Source_Clock();
	Init_Delay();
	Init_SPI1();
	Init_ADC1();
	Init_NRF24L01();
	NRF24L01_TX_mode(Address_TX, 10);
	while(1)
	{
		Read_ADC(&Joystick_X, &Joystick_Y);
		if(Joystick_X > 3500)
		{
			memcpy(TX_Data, "toward",sizeof("toward"));
			if(Joystick_Y > 3500)
				memcpy(TX_Data, "rightward",sizeof("rightward"));
			else if(Joystick_Y < 1500)
				memcpy(TX_Data, "leftward",sizeof("leftward"));
		}
		else if(Joystick_X < 1500)
		{
			memcpy(TX_Data, "backward",sizeof("backward"));
			if(Joystick_Y > 3500)
				memcpy(TX_Data, "rightward",sizeof("rightward"));
			else if(Joystick_Y < 1500)
				memcpy(TX_Data, "leftward",sizeof("leftward"));
		}
		else if(Joystick_Y > 3500)
		{
			memcpy(TX_Data, "rightward",sizeof("rightward"));
			if(Joystick_X > 3500)
				memcpy(TX_Data, "toward",sizeof("toward"));
			else if(Joystick_X < 1500)
				memcpy(TX_Data, "backward",sizeof("backward"));
		}
		else if(Joystick_Y < 1500)
		{
			memcpy(TX_Data, "leftward",sizeof("leftward"));
			if(Joystick_X > 3500)
				memcpy(TX_Data, "toward",sizeof("toward"));
			else if(Joystick_X < 1500)
				memcpy(TX_Data, "backward",sizeof("backward"));
		}
		else
		{
			memcpy(TX_Data, "balance",sizeof("balance"));
		}
		NRF24L01_Transmit(TX_Data);
	}
	return 1;
}
/**************************************************************************************************************/

