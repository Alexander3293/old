/*
 * write_flash.c
 *
 *  Created on: 10 февр. 2020 г.
 *      Author: Александр
 */
#include "write_flash.h"

/*
 * Пишем во Flash память значения для конфигурации RCC
 * Их считываем по Ethernet, чистим Flash и пишем
 * При включении проверяем, записано ли там что-то. Смотрим после SystemClock_Config()
 *
 * */
extern ADC_HandleTypeDef hadc1;

int config_rcc(uint8_t *Data8)
{
	//uint8_t reading_Data[512]={0,};
	//flash_stm32_read_byte(reading_Data,256, 0x08007C00);
	/* Отчищаем все перед с записью*/
	//uint32_t configure_rcc = 0x124812; //Сюда пишем значение для конфигурации RCC

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES; //Стираем только страницу
    EraseInitStruct.PageAddress = 0x08007C00;			 //Последняя страница 31 (нумерация с 0), тип 32
    EraseInitStruct.NbPages     = 1; 					 //Число страниц лдя стирания - 1
    HAL_FLASH_Unlock();   // Разблокируем флеш память
    HAL_FLASHEx_Erase(&EraseInitStruct,&PAGEError);
    //HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x08007C00, configure_rcc );   // Записываем значение переменной isTimeWorkL на 63 странице флеш памяти
    for(int i=0; i<255; i++) HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 0x08007C00,(uint32_t) Data8[i] );   // Записываем значение переменной isTimeWorkL на 63 странице флеш памяти
    //HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 0x08007C00,(uint32_t) Data8[k] );   // Записываем значение переменной isTimeWorkL на 63 странице флеш памяти
    HAL_FLASH_Lock();   // Блокируем флеш память

    return 1;
}

void flash_stm32_read_byte(uint8_t *Data8, const uint32_t SizeBytes, const uint32_t AddrStart)
{
	uint32_t Address = AddrStart;

	while(Address < (SizeBytes + AddrStart))
	{
		Data8[Address - AddrStart] = *(__IO uint8_t *)Address;
		++Address;
	}
}


void proba_config(uint8_t *Data)
{
	 /*
	  * За частоту отвечают:
	  * SYSCLK RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL7; т.е. 7 * частоту HSE
	  *  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2; //предделитель HSE
	  *  он делает из внешнего 8, делает 4
	  * AHB Prescaler  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV8;
	  * APB2 Prescaler RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;
	  * ADC Prescaler  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
	  */
}


//void EXTI3_IRQHandler(void)
//{
//  /* USER CODE BEGIN EXTI3_IRQn 0 */
//
//  /*
//   *  1) Запуск АЦП от фронта pps
//   *
//   *  2) Запуск снятия данных в момент, когда приняли фронт
//   *
//   */
//
//	cnt_adc_1ppt=0;
//	if(use_hse) //если в Qt выбрали  HSE
//	{
//		current_time = HAL_GetTick();
//		//__HAL_TIM_SET_COUNTER(&htim3, 0x00);//По принятию прерывания, обнулить Таймер
//		if(flag_adc_stop)
//		{ //если запуск после остановки, то врубить таймер
//			HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
//			HAL_TIM_Base_Start(&htim3);
//			flag_adc_stop = RESET;
//		}
//
//
//	}
//	else        //запуск от внешнего
//	{
//		current_time = HAL_GetTick();
//		if(flag_adc_stop)
//		{ //если запуск после остановки, то врубить таймер
//			HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
//			flag_adc_stop = RESET;
//		}
//
//	}
//
//
//  /* USER CODE END EXTI3_IRQn 0 */
//  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
//  /* USER CODE BEGIN EXTI3_IRQn 1 */
//
//  /* USER CODE END EXTI3_IRQn 1 */
//}
