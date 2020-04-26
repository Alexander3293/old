/*
 * wizchip_init.h
 *
 *  Created on: 12 янв. 2020 г.
 *      Author: Александр
 */

#ifndef WIZCHIP_INIT_H_
#define WIZCHIP_INIT_H_

#include "socket.h"
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>  //Для atoi()
///////////////////////////////////////////////
//Select the library for your stm32fxxx_hal.h//
///////////////////////////////////////////////

#include "stm32f1xx_hal.h"

//#include "write_flash.h"
#include "main.h"

/////////////////////////////
//Write the necessary ports//
/////////////////////////////

#define P_SCK  		GPIOA, GPIO_PIN_5             //SPI_SCK
#define P_MISO 		GPIOA, GPIO_PIN_6		     //SPI_MISO
#define P_MOSI 		GPIOA, GPIO_PIN_7			 //SPI_MOSI
//-----------------------------------------------------------------------------//

//////////////////////////////////////////////////
//Write  GPIOx peripheral and specifies the port//
//bit GPIO_PINx to be written for SPI CS 		//
//////////////////////////////////////////////////

#define GPIOx_SPI_CS				GPIOA, GPIO_PIN_4
//-------------------------------------------------------------------------------//

#define WIZCHIP_SPI_CLK_PIN   P_SCK     //SPI_SCK
#define WIZCHIP_SPI_MISO_PIN  P_MISO	  //SPI_MISO
#define WIZCHIP_SPI_MOSI_PIN  P_MOSI	  //SPI_MOSI


/////////////////////////////////////////
// SOCKET NUMBER DEFINION for Examples //
/////////////////////////////////////////
#define SOCK_TCPS        0
#define SOCK_UDPS        1

////////////////////////////////////////////////
// Shared Buffer Definition for LOOPBACK TEST //
////////////////////////////////////////////////
#define DATA_BUF_SIZE   2048

#define FLAG_READY_SEND_HALF_DM 3
#define FLAG_READY_SEND_FULL_DM 4
//extern uint8_t volatile FLAG_READY_SEND_HALF_DM;
//extern uint8_t volatile FLAG_READY_SEND_FULL_DM ;

//#define BUFSIZE 1460 // Не может сформировать пакеты больше, шлет частями 1460
#define BUFSIZE 1010
#define PREAMBULA 10
#define BUFSIZE_ADC (BUFSIZE-PREAMBULA) // -10 так, как на пакет 10 байтов

////////////////////////////////////////////////
#define FREQUENCE_1_kHz_adc1	0xb1
#define FREQUENCE_10_kHz_adc1	0xb4
#define FREQUENCE_50_kHz_adc1	0xb7

#define FREQUENCE_1_kHz_adc2	0xc1
#define FREQUENCE_10_kHz_adc2	0xc4
#define FREQUENCE_50_kHz_adc2	0xc7

#define OUT_TAKT_adc1   		0xa1 //внешнее тактирование АЦП1 второго канала
#define OUT_TAKT_adc2           0xa3 //внешнее тактирование АЦП1 третьего канала
#define HSE_TIMER       		0xa7 //внутреннее HSE от таймера

#define SINGLE_ADC2				0xa9 //Однократное измерение АЦП2
////////////////////////////////////////////////

extern volatile uint8_t flag_dma_send;
extern volatile uint16_t cnt_adc_1ppt;
extern volatile uint8_t flag_adc_stop;
extern volatile uint16_t current_time;
extern uint16_t rx_buff_ADC_DM[BUFSIZE_ADC];  //DMA buffer

extern volatile uint8_t use_hse; //ЧТО в Qt выбрали HSE

//struct {
//
//}

/* Private function prototypes -----------------------------------------------*/
void  wizchip_select(void);
void  wizchip_deselect(void);
void  wizchip_write(uint8_t wb);
uint8_t wizchip_read();

void UART_Printf(const char* fmt, ...);
void WizchIP_main(SPI_HandleTypeDef* spi, UART_HandleTypeDef* uart, ADC_HandleTypeDef* adc);
//////////////////////////////////
// For example of ioLibrary_BSD //
//////////////////////////////////
void network_init(void);								// Initialize Network information and display it
int32_t loopback_tcps(uint8_t, uint8_t*, uint16_t);		// Loopback TCP server
int32_t loopback_udps(uint8_t, uint8_t*, uint16_t);		// Loopback UDP server
int32_t WIZ55_TCP_Client_Connect(uint8_t sn,uint8_t* buf, uint16_t port); // TCP client

//////////////////////////////////
void choose_sub(void);
void choose_ip(void);
int8_t func(uint8_t *packet_2, uint16_t *rx_buff_ADC_DMA );

void hse_1_kHz(void);     //Внутренний от 1 кГЦ
void hse_10_kHz(void);    //Внутренний от 1 кГЦ
void hse_50_kHz(void);   //Внутренний от 1 кГЦ
void config_in_adc1(void); //Конфигурируем АЦП1 для запуска от таймера
void config_in_adc2(void); //Конфигурируем АЦП2 для запуска от таймера
void config_out_adc1(void);//Конфигурируем АЦП1 для запуска от внешнего сигнала EXTI 11
void config_out_adc2(void);//Конфигурируем АЦП2 для запуска от внешнего сигнала EXTI 11
void start_multimode(uint32_t *rx_buff);

void reboot(void);

#endif /* WIZCHIP_INIT_H_ */
