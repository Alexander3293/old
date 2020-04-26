/*
 * write_flash.c
 *
 *  Created on: 10 ����. 2020 �.
 *      Author: ���������
 */
#include "write_flash.h"

/*
 * ����� �� Flash ������ �������� ��� ������������ RCC
 * �� ��������� �� Ethernet, ������ Flash � �����
 * ��� ��������� ���������, �������� �� ��� ���-��. ������� ����� SystemClock_Config()
 *
 * */
extern ADC_HandleTypeDef hadc1;

int config_rcc(uint8_t *Data8)
{
	//uint8_t reading_Data[512]={0,};
	//flash_stm32_read_byte(reading_Data,256, 0x08007C00);
	/* �������� ��� ����� � �������*/
	//uint32_t configure_rcc = 0x124812; //���� ����� �������� ��� ������������ RCC

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES; //������� ������ ��������
    EraseInitStruct.PageAddress = 0x08007C00;			 //��������� �������� 31 (��������� � 0), ��� 32
    EraseInitStruct.NbPages     = 1; 					 //����� ������� ��� �������� - 1
    HAL_FLASH_Unlock();   // ������������ ���� ������
    HAL_FLASHEx_Erase(&EraseInitStruct,&PAGEError);
    //HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x08007C00, configure_rcc );   // ���������� �������� ���������� isTimeWorkL �� 63 �������� ���� ������
    for(int i=0; i<255; i++) HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 0x08007C00,(uint32_t) Data8[i] );   // ���������� �������� ���������� isTimeWorkL �� 63 �������� ���� ������
    //HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 0x08007C00,(uint32_t) Data8[k] );   // ���������� �������� ���������� isTimeWorkL �� 63 �������� ���� ������
    HAL_FLASH_Lock();   // ��������� ���� ������

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
	  * �� ������� ��������:
	  * SYSCLK RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL7; �.�. 7 * ������� HSE
	  *  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2; //������������ HSE
	  *  �� ������ �� �������� 8, ������ 4
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
//   *  1) ������ ��� �� ������ pps
//   *
//   *  2) ������ ������ ������ � ������, ����� ������� �����
//   *
//   */
//
//	cnt_adc_1ppt=0;
//	if(use_hse) //���� � Qt �������  HSE
//	{
//		current_time = HAL_GetTick();
//		//__HAL_TIM_SET_COUNTER(&htim3, 0x00);//�� �������� ����������, �������� ������
//		if(flag_adc_stop)
//		{ //���� ������ ����� ���������, �� ������� ������
//			HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
//			HAL_TIM_Base_Start(&htim3);
//			flag_adc_stop = RESET;
//		}
//
//
//	}
//	else        //������ �� ��������
//	{
//		current_time = HAL_GetTick();
//		if(flag_adc_stop)
//		{ //���� ������ ����� ���������, �� ������� ������
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
