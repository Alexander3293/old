/*
 * wizchip_init.c
 *
 *  Created on: 12 янв. 2020 г.
 *      Author: Александр
 */

/*
 * Для работы с данной билиотекой, необходимо настроить порты SPI в
 * файле wizchip_functions.h SPI_SCK, SPI_MISO, SPI_MOSI и GPIOx_SPI_CS,
 * а также указать hal - библиотеку микропроцессора  stm32
 * (например, stm32f4xx_hal.h).
 *
 * Для инциализации в main.c указать после
 *
 * / Initialize all configured peripherals /
 *
 *   WizchIP_main(&hspi1, &huart2);
 *  где в аргументах функции содержится адрес периферии SPI и UART_2.
 *
 *  Прием данных с UART по прерыванию, для это в файле /../Drivers/Src/stm32f1xx_hal_uart.c
 *  в конце функции void HAL_UART_IRQHandler(UART_HandleTypeDef *huart) добавить:
 *   // UART RX IDLE interrupt --------------------------------------------
    if(((isrflags & USART_SR_IDLE) != RESET) && ((cr1its & USART_CR1_IDLEIE) != RESET))
    {
        HAL_UART_IDLE_Callback(huart);
        return;
    }
 *
 * В функции void WizchIP_main(SPI_HandleTypeDef* spi, UART_HandleTypeDef* uart) имеются
 * две функции 	choose_ip()и choose_sub() - они необходимы для смены IP адреса и маски подсети
 * WIZCHIP, либо можно закомментировать эти две функции и поменять вручную в структуре
 * wiz_NetInfo gWIZNETINFO.
 *
 *АЦП: Имеется возможность работы с АЦП в режиме DMA от таймера рассчитанного на 1кГц, 10 и 50 кГц
 *АЦП: Задается функцией hse_1_kHz и т.д и от Внешнего сигнала, расположенного на PB11
 *АЦП: Частоты или внешний выбираются с приложения на компьютере и вызываются в socket receive.
 *АЦП: АЦП 12битное, получаем т.е. 16 бит, разбиваем по 8 бит
 *
 *Ethernet: Отправляем максимум 1460 байт, преамбула 10 байт, и остальное - данные
 *Ethernet: Макс - 50 кГц, т.к. отправка 1010 байт по Eth занимает 5мс.
 */

#include "wizchip_init.h"


SPI_HandleTypeDef* SPI_WIZCHIP;
UART_HandleTypeDef* UART_WIZCHIP;
ADC_HandleTypeDef* hadc;

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;		//Костыль - переделать

extern TIM_HandleTypeDef htim3;
//uint8_t volatile FLAG_READY_SEND_HALF_DM = 3;
//uint8_t volatile FLAG_READY_SEND_FULL_DM = 4;

extern uint8_t flag_send_adc;
volatile uint8_t flag_adc_stop = SET;
volatile uint8_t use_hse = RESET;
uint32_t ticks_system = 0; //Системный таймер
uint32_t ticks_second = 0; //Сохраняет значение таймера после прерывания
bool FLAG = RESET;
bool FLAG_TCP = RESET; //Чтобы перенаправлять потоки на TCP и UDP
volatile uint16_t cnt_adc_1ppt=0; //Если насчитал нужное количество отсчетов для 1 pps/
volatile uint16_t current_time = 0;
uint16_t max_cnt_adc_1ppt = 0;

bool choose_adc =0;

volatile uint8_t flag_dma_send=0;

uint16_t rx_buff_ADC_DM[BUFSIZE_ADC] = {0,};  //DMA buffer

uint16_t rx_buff_uart_len;
uint8_t rx_buff_uart[30]={0,};

uint16_t adc_proba = 0;
int counter_adc_massiv=0;

uint8_t  udp_destip[4];
uint16_t udp_destport;

//uint8_t packet_2[BUFSIZE];


uint16_t counter_time = 0;

//wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc,0x00, 0xab, 0xcd},
		wiz_NetInfo gWIZNETINFO = { .mac = {	0x4B,0xAA,0xBB,0xCC,0xDD,0xEE},
							.ip = {169, 254, 153, 204},
                            .sn = {255,255,0,0},
                            .gw = {0,0,0,0},
                            .dns = {0,0,0,0},
                            .dhcp = NETINFO_STATIC };

void WizchIP_main(SPI_HandleTypeDef* spi, UART_HandleTypeDef* uart, ADC_HandleTypeDef* adc)
{

	SPI_WIZCHIP = spi;
	UART_WIZCHIP = uart;
	hadc = adc;
	uint8_t gDATABUF[DATA_BUF_SIZE];
	HAL_NVIC_DisableIRQ(EXTI1_IRQn);



    //	choose_ip();
    //	choose_sub();
	//   __HAL_UART_ENABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
	//   HAL_UART_Receive_IT(UART_WIZCHIP, (uint8_t*)rx_buff_uart, BUFSIZE);


	uint8_t tmp;
	int32_t ret = 0;
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}}; //из main
	//------------------------------------------------------------------------------

	 // First of all, Should register SPI callback functions implemented by user for accessing WIZCHIP //
	   ////////////////////////////////////////////////////////////////////////////////////////////////////
	   /* Critical section callback - No use in this example */
	   reg_wizchip_cris_cbfunc(0, 0);
	   /* Chip selection call back */
	#if   _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_VDM_
	    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	#elif _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_FDM_
	    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_select);  // CS must be tried with LOW.
	#else
	   #if (_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SIP_) != _WIZCHIP_IO_MODE_SIP_
	      #error "Unknown _WIZCHIP_IO_MODE_"
	   #else
	      reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	   #endif
	#endif
	    /* SPI Read & Write callback function */
	    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	    ////////////////////////////////////////////////////////////////////////
	    /* WIZCHIP SOCKET Buffer initialize */
	        if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
	        {

	        	//UART_Printf("WIZCHIP Initialized fail.\r\n");


	           while(1);
	        }

	        /* PHY link status check */
	        do
	        {
	           if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)

	        	   UART_Printf("Unknown PHY Link stauts.\r\n");

	        }while(tmp == PHY_LINK_OFF);
	        /* Network initialization */
	            network_init();
	            /*******************************/
	            	/* WIZnet W5500 Code Examples  */
	            	/* TCPS/UDPS Loopback test     */
	            	/*******************************/
	                /* Main loop */

	                while(1)
	            	{

//	                	if( (ret = WIZ55_TCP_Client_Connect(1,gDATABUF, 80)) < 0)
//	                	{
//
//	                		//UART_Printf("SOCKET ERROR : %ld\r\n", ret);
//
//
//	                	}

	                	// TCP server loopback test
	                	if( (ret = loopback_tcps(SOCK_TCPS, gDATABUF, 5000)) < 0)
	                	{
	                		//UART_Printf("SOCKET ERROR : %ld\r\n", ret);
	            		}

	                	// UDP server loopback test
//	            		if( (ret = loopback_udps(SOCK_UDPS, gDATABUF, 3000)) < 0) {
//
//	            			//UART_Printf("SOCKET ERROR : %ld\r\n", ret);
//	            		}
	            	} // end of Main loop

}



void  wizchip_select(void)
{
   HAL_GPIO_WritePin(GPIOx_SPI_CS, GPIO_PIN_RESET);

}

void  wizchip_deselect(void)
{
	HAL_GPIO_WritePin(GPIOx_SPI_CS, GPIO_PIN_SET);
}


void  wizchip_write(uint8_t wb)    //Write SPI
{
	HAL_SPI_Transmit( SPI_WIZCHIP, &wb, 	1 , HAL_MAX_DELAY);
}

uint8_t wizchip_read() //Read SPI
{
	uint8_t spi_read_buf;
    HAL_SPI_Receive (SPI_WIZCHIP, &spi_read_buf, 1, HAL_MAX_DELAY);
    return spi_read_buf;

}



/////////////////////////////////////////////////////////////
// Intialize the network information to be used in WIZCHIP //
/////////////////////////////////////////////////////////////
void network_init(void)
{
	uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
   uint8_t tmpstr[6];
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);

	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);

//	HAL_Delay(50);
//	UART_Printf("\r\n=== %s NET CONF ===\r\n",(char*)tmpstr);
//	HAL_Delay(10);
//	UART_Printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],
//			  gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
//	HAL_Delay(10);
//	UART_Printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
//	HAL_Delay(10);
//	UART_Printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
//	HAL_Delay(10);
//
//	UART_Printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
//	HAL_Delay(10);
//	UART_Printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
//	UART_Printf("======================\r\n");

}
/////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////
// Loopback Test Example Code using ioLibrary_BSD			 //
///////////////////////////////////////////////////////////////
int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port)
{
   int32_t ret;
   uint16_t size = 0, sentsize=0;
   uint8_t packet[2*BUFSIZE]={0,};
   bool flag = SET; //Чтобы после подключения начинало работать DMA




   switch(getSn_SR(sn))   //Проверить состояние сокета sn
   {
      case SOCK_ESTABLISHED :	//Успешное подключение
    	  if(flag)
    	  {
    		  //HAL_TIM_Base_Start(&htim3);
    		  //HAL_ADC_Start(hadc);

    		  // BUFFER DMA rx_buff_ADC_DMA[BUFSIZE_ADC]
    		  flag = RESET;
    	  }

         if(getSn_IR(sn) & Sn_IR_CON)
         {

        	 //UART_Printf("%d:Connected\r\n",sn);
        	 //FLAG_TCP = SET; FLAG == SET ;
            setSn_IR(sn,Sn_IR_CON);
         }
         ticks_second = 0;
        //Если разность больше чем 10сек, то вырубить периферию
        // Долго не было 1pps
     	//if((HAL_GetTick() - ticks_second) >= 100 )

        /*
         * Здесь по прерыванию должен заново запускать АЦП и
         * флагами выставить, что запускается от таймера или внешним
         */
         	 if(flag_adc_stop && counter_time==0)
         	 {
         		//__HAL_UNLOCK(hadc);
         		 //HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&hadc->Instance->DR, (uint32_t)&rx_buff_ADC_DM, BUFSIZE_ADC);
         		 //flag_adc_stop =RESET;
         		  //HAL_TIM_Base_Start(&htim3); //OFF in hse_1_kHz
         		//__HAL_LOCK(hadc);
         	 }

            if(func(packet,rx_buff_ADC_DM)== 1)//Формируем пакет
            {

                 ret = send(sn, packet, BUFSIZE);//половина DMA и шапка
				 flag_dma_send=0;
				 cnt_adc_1ppt++;



            }


            if(((HAL_GetTick() - current_time ) >= 1018)&& !flag_adc_stop)
            {

            	/*Конструкция отключает таймер, если долго нет прерывания*/
            	if(use_hse)
            	{

            		HAL_ADC_Stop_DMA(hadc);
            		HAL_TIM_Base_Stop(&htim3);
                	flag_adc_stop = SET;
            	}
                /*Если тактируем от внешнего сигнала и время после последнего
                 * прерывания превысило 2000 ms*/
            	else if(!use_hse)
            	{
                	HAL_ADC_Stop_DMA(hadc);
                	flag_adc_stop = SET;
            	}
            }

         if((size = getSn_RX_RSR(sn)) > 0)
         {

            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = recv(sn,buf,size);
            if(ret <= 0) return ret;
            sentsize = 0;


        	if(buf[0] == HSE_TIMER)
        	{
        		switch(buf[1])  // Настраиваем TIMER
        		{
        		case FREQUENCE_1_kHz_adc1 :

        			choose_adc = 0;
        			hse_1_kHz();

        			break;
        		case FREQUENCE_1_kHz_adc2 :

        			//hadc = &hadc2;
        			choose_adc = 1;
        			hse_1_kHz();

        			break;
        		case FREQUENCE_10_kHz_adc1 :

        			choose_adc = 0;
        			hse_10_kHz();

        			break;
        		case FREQUENCE_10_kHz_adc2 :

        			choose_adc = 1;
        			hse_10_kHz();

        			break;
        		case FREQUENCE_50_kHz_adc1 :

        			choose_adc = 0;
        			hse_50_kHz();

        			break;
        		case FREQUENCE_50_kHz_adc2 :
        			choose_adc = 1;
        			hse_50_kHz();

        			break;
        		case SINGLE_ADC2:
        			HAL_ADCEx_Calibration_Start(&hadc2);
        			HAL_ADC_Start(&hadc2);
        			HAL_ADC_PollForConversion(&hadc2,-1); //Время преобразования
        			uint32_t value_adc = HAL_ADC_GetValue(&hadc2);
        			uint8_t value_adc_send[3]={0,};

        			value_adc_send[0]=(value_adc & 0xFF00) >> 8;
        			value_adc_send[1]=(uint8_t)value_adc;

        			ret = send(sn, value_adc_send, 2);

        			break;
        		default:
        			//Error_Handler();
        			break;
        		}

        	}
        	//внешнее тактирование
        	else if(buf[0] == OUT_TAKT_adc1)
        	{
        		config_out_adc1();
        	}
        	else if(buf[0] == OUT_TAKT_adc2)
        	{
        		config_out_adc2();
        	}

        	memset(buf, 0, size);
            while(size != sentsize)
            {
              // HAL_UART_Transmit(UART_WIZCHIP, buf+sentsize, size-sentsize, HAL_MAX_DELAY);
               if(ret < 0)
               {
                  close(sn);
                  return ret;
               }
               sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
         }
         break;
      case SOCK_CLOSE_WAIT :

         //UART_Printf("%d:CloseWait\r\n",sn);
         if((ret=disconnect(sn)) != SOCK_OK) return ret;
         //UART_Printf("%d:Closed\r\n",sn);

         break;
      case SOCK_INIT :

    	  ////UART_Printf("%d:Listen, port [%d]\r\n",sn, port);
         if( (ret = listen(sn)) != SOCK_OK) return ret;

         break;
      case SOCK_CLOSED:

    	  reboot(); //Отключаем АЦП И ТАЙМЕР
    	  //UART_Printf("%d:LBTStart\r\n",sn);
    	  flag = SET;
      	  flag_adc_stop = SET;

          if((ret=socket(sn,Sn_MR_TCP,port,0x00)) != sn)  return ret;


         //HAL_ADC_Start(hadc); //
         //HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DMA, BUFSIZE_ADC/2); //  ADC_DMA



//         __HAL_UART_ENABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
//         HAL_UART_Receive_IT(UART_WIZCHIP, (uint8_t*)rx_buff_uart, BUFSIZE);
         //UART_Printf("%d:Opened\r\n",sn);
         break;
      default:
         break;
   }
   return 1;
}

int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port)
{
   int32_t  ret;
   uint16_t size, sentsize;
   uint8_t  destip[4];
   uint16_t destport;
   //uint8_t  packinfo = 0;
   switch(getSn_SR(sn))
   {
      case SOCK_UDP :

          if(FLAG == SET && FLAG_TCP == RESET && udp_destport>0 )
          {
        	  ret = sendto(sn,rx_buff_uart,rx_buff_uart_len,udp_destip,udp_destport);
        	  if(ret < 0)
        	  {

        	 //   //UART_Printf("%d: sendto error. %ld\r\n",sn,ret);
        	    return ret;
        	  }
              FLAG = RESET;

          }

         if((size = getSn_RX_RSR(sn)) > 0)
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;

            ret = recvfrom(sn,buf,size,destip,(uint16_t*)&destport);

            memcpy(udp_destip,destip, 4);
            udp_destport = port;
           // ret = sendto(sn,buf,size,destip,(uint16_t*)&destport);

            if(ret <= 0)
            {

            	//UART_Printf("%d: recvfrom error. %ld\r\n",sn,ret);
               return ret;
            }
            size = (uint16_t) ret;
            sentsize = 0;
            while(sentsize != size)
            {

                HAL_UART_Transmit(UART_WIZCHIP, buf+sentsize, size-sentsize,
                                     HAL_MAX_DELAY);
             sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
         }
         break;
      case SOCK_CLOSED:

         //UART_Printf("%d:LBUStart\r\n",sn);
         if((ret=socket(sn,Sn_MR_UDP,port,0x00)) != sn)
            return ret;

      //UART_Printf("%d:Opened, port [%d]\r\n",sn, port);
         break;
      default :
         break;
   }
   return 1;
}

void UART_Printf(const char* fmt, ...) {
    char buff[256]; //сюда пишем рез-т
    va_list args;  //переменные параметры
    va_start(args, fmt); //макрос, который считает, что все параметры
    //после fmt - переменные параметры
    vsnprintf(buff, sizeof(buff), fmt, args);
    HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)buff, strlen(buff),
                      10);
    va_end(args);
}

void HAL_UART_IDLE_Callback(UART_HandleTypeDef *huart)
{
	if(huart == UART_WIZCHIP)
	{
		__HAL_UART_DISABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
		rx_buff_uart_len = BUFSIZE - huart->RxXferCount;
		uint8_t res = HAL_UART_Transmit_IT(UART_WIZCHIP, (uint8_t*)rx_buff_uart, rx_buff_uart_len);

		FLAG = SET;

		if(res == HAL_ERROR) HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"HAL_ERROR - rx_buff == NULL or rx_buff_len == 0\n", 48, 1000);
		else if(res == HAL_BUSY) HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"HAL_BUSY\n", 9, 1000);
		HAL_UART_AbortReceive(UART_WIZCHIP);
		__HAL_UART_CLEAR_IDLEFLAG(UART_WIZCHIP);
		__HAL_UART_ENABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
		HAL_UART_Receive_IT(UART_WIZCHIP, (uint8_t*)rx_buff_uart, BUFSIZE);
	}
}

/////////////////////////////////// полный буфер ///////////////////////////////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	  if(huart == UART_WIZCHIP)
	  {
		 // __HAL_UART_DISABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
		  HAL_UART_Transmit_IT(UART_WIZCHIP, rx_buff_uart, BUFSIZE);
		  rx_buff_uart_len = BUFSIZE - huart->RxXferCount;
		  FLAG = SET;

		  HAL_UART_AbortReceive(UART_WIZCHIP);
		  __HAL_UART_CLEAR_IDLEFLAG(UART_WIZCHIP);
		  //__HAL_UART_ENABLE_IT(UART_WIZCHIP, UART_IT_IDLE);
		  HAL_UART_Receive_IT(UART_WIZCHIP, (uint8_t*)rx_buff_uart, BUFSIZE);
	  }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if(huart == UART_WIZCHIP)
	{
		__HAL_UART_DISABLE_IT(UART_WIZCHIP, UART_IT_IDLE);

		uint32_t er = HAL_UART_GetError(UART_WIZCHIP);
		HAL_UART_Abort_IT(UART_WIZCHIP);

		switch(er)
		{
			case HAL_UART_ERROR_PE:
				HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"ERR_Callbck - Parity error\n", 27, 1000);
				__HAL_UART_CLEAR_PEFLAG(UART_WIZCHIP);
				huart->ErrorCode = HAL_UART_ERROR_NONE;
			break;

			case HAL_UART_ERROR_NE:
				HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"ERR_Callbck - Noise error\n", 26, 1000);
				__HAL_UART_CLEAR_NEFLAG(UART_WIZCHIP);
				huart->ErrorCode = HAL_UART_ERROR_NONE;
			break;

//			case HAL_UART_ERROR_FE:
//				HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"ERR_Callbck - Frame error\n", 26, 1000);
//				__HAL_UART_CLEAR_FEFLAG(UART_WIZCHIP);
//				huart->ErrorCode = HAL_UART_ERROR_NONE;
			break;

			case HAL_UART_ERROR_ORE:
				HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"ERR_Callbck - Overrun error\n", 28, 1000);
				__HAL_UART_CLEAR_OREFLAG(huart);
				huart->ErrorCode = HAL_UART_ERROR_NONE;
			break;

			case HAL_UART_ERROR_DMA:
				HAL_UART_Transmit(UART_WIZCHIP, (uint8_t*)"ERR_Callbck - DMA transfer error\n", 33, 1000);
				huart->ErrorCode = HAL_UART_ERROR_NONE;
			break;

			default:
			break;
		}
	}

}
/*Configuration IP, if use UART*/
void choose_ip(void)
{
	//UART_Printf("Choose IP( write 1) or 0 for default 169.254.153.204 \r\n");
		uint8_t choose_buf[1]={2};


		//------------------------------------------------------------------------------

		HAL_Delay(10);

		while(HAL_UART_Receive(UART_WIZCHIP,(uint8_t*)choose_buf, 1, 1000)!= HAL_OK);


		if(choose_buf[0]=='1')
		{
			//UART_Printf( "Write IP: example: 169.254.XXX.XXX \r\n");


			int flag_receive = SET;
			int i=0;
			char choose_sub[17]={0,};

			int counter_sub = 0;
			uint8_t rec_buf[1]={0,};
			while(flag_receive == SET)
			{

					if(HAL_UART_Receive(UART_WIZCHIP,(uint8_t*)rec_buf, 1, 1000)== HAL_OK)
						{

						choose_sub[i] = rec_buf[0];
							i++;

								if(strlen(choose_sub) >= 15)
								{
								char rec_buf[4]={0,};
								int i=0;
								int m=0;
								while(counter_sub!=4)
								{
									rec_buf[m] = choose_sub[i];
									if(m==2)
									{

										gWIZNETINFO.ip[counter_sub]= atoi(rec_buf);
										counter_sub++;

										m=-1;
										i++;

										memset(rec_buf, 0, 4);

									}
									m++; i++;


								}
								}
								if(counter_sub==4) break;
						}
			}

	}
		//UART_Printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
}

/*Configuration SubNet Mask, if use UART*/
void choose_sub(void)
{
	//UART_Printf("Choose SUB( write 1) or 0 for default 255.255.0.0 \r\n");
		uint8_t choose_buf[1]={2};


		//------------------------------------------------------------------------------

		HAL_Delay(10);

		while(HAL_UART_Receive(UART_WIZCHIP,(uint8_t*)choose_buf, 1, 1000)!= HAL_OK);


		if(choose_buf[0]=='1')
		{
			//UART_Printf( "Write SUB: example: 255.255.000.000 \r\n");


			int flag_receive = SET;
			int i=0;
			char choose_sub[17]={0,};

			int counter_sub = 0;
			uint8_t rec_buf[1]={0,};
			while(flag_receive == SET)
			{

					if(HAL_UART_Receive(UART_WIZCHIP,(uint8_t*)rec_buf, 1, 1000)== HAL_OK)
						{

						choose_sub[i] = rec_buf[0];
							i++;

								if(strlen(choose_sub) >= 15)
								{
								char rec_buf[4]={0,};
								int i=0;
								int m=0;
								while(counter_sub!=4)
								{
									rec_buf[m] = choose_sub[i];
									if(m==2)
									{

										gWIZNETINFO.sn[counter_sub]= atoi(rec_buf);
										counter_sub++;

										m=-1;
										i++;

										memset(rec_buf, 0, 4);

									}
									m++; i++;


								}
								}
								if(counter_sub==4) break;
						}
			}

	}
//UART_Printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
}



int8_t func(uint8_t *packet_2, uint16_t *rx_buff_ADC_DMA )
{

		uint32_t time_clock = (rx_buff_uart[0]<<24)|(rx_buff_uart[1]<<16)|(rx_buff_uart[2]<<8)|(rx_buff_uart[3]);

		if(flag_dma_send ==FLAG_READY_SEND_HALF_DM)
		{
			HAL_NVIC_DisableIRQ(EXTI1_IRQn);
			 //adc_to_ethernet(hadc);
			int counter_dma = PREAMBULA;
			int i=0;
			while(i != (BUFSIZE_ADC/2))
			{
				packet_2[counter_dma] = (rx_buff_ADC_DMA[i] & 0xFF00) >> 8;
				packet_2[counter_dma+1] = ((uint8_t)rx_buff_ADC_DMA[i]);

				i++;
				counter_dma+=2;
			}

			packet_2[0] = 0x00;
			packet_2[1] = 0x00;
	 		packet_2[2] = 0xea;
	 		packet_2[3] = 0x60;

     		packet_2[4] = time_clock >> 24;
     		packet_2[5] = time_clock >> 16;
     		packet_2[6] = time_clock >> 8;
     		packet_2[7] = time_clock;

     		packet_2[8]= counter_time >> 8;
     		packet_2[9]= counter_time;
     		counter_time++;
     		HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
     		return 1;

			//flag_dma_send==RESET;
		}
	if(flag_dma_send == FLAG_READY_SEND_FULL_DM)
	{
		HAL_NVIC_DisableIRQ(EXTI1_IRQn);
		int counter_dma = PREAMBULA;
		int i=0; //Начинаем с половины буффера rx_buff_ADC_DMA, т.к. вторая половина
		//int i=0; //Начинаем с половины буффера rx_buff_ADC_DMA, т.к. вторая половина
		while(i != (BUFSIZE_ADC/2)) //Весь DMA BUFSIZE_ADC/2, начинаем с BUFSIZE/4
		{

			packet_2[counter_dma] = (rx_buff_ADC_DMA[i + BUFSIZE_ADC/2] & 0xFF00) >> 8;
			packet_2[counter_dma+1] = ((uint8_t)rx_buff_ADC_DMA[i + BUFSIZE_ADC/2]);

			i++;
			counter_dma+=2;
		}

		packet_2[0] = 0x00;
		packet_2[1] = 0x00;
 		packet_2[2] = 0xea;
 		packet_2[3] = 0x60;

 		packet_2[4] = 0x10;
 		packet_2[5] = 0x11;
 		packet_2[6] = 0x12;
 		packet_2[7] = 0x13;

 		packet_2[8]= counter_time >> 8;
 		packet_2[9]= counter_time;
 		counter_time++;
 		HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
		return 1;
		//flag_dma_send==RESET;
	}

		return 0;

}

/* Запуск АЦП от таймера и конфигурация таймера под нужную частоту*/
void hse_1_kHz(void)
{

	  if(!choose_adc) config_in_adc1();

	  else if(choose_adc)  config_in_adc2();
	 config_in_adc1();
	  HAL_TIM_Base_Stop(&htim3);
	  HAL_TIM_Base_DeInit(&htim3);

	  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	  TIM_MasterConfigTypeDef sMasterConfig = {0};

	  htim3.Instance = TIM3;
	  htim3.Init.Prescaler = 1;
	  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	  htim3.Init.Period = 35999; //Т.к. 72МГц / 2 = 36МГц
	  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
	  use_hse = SET;
	  flag_adc_stop = SET;
	  max_cnt_adc_1ppt = 2; //2 пакета по 1000 отссылает, в каждом из них по 500 отсчетов (12бит)
	 // HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
	 // HAL_TIM_Base_Start(&htim3);	//Запуск в прерывании



}

void hse_10_kHz(void)
{
	  if(!choose_adc) config_in_adc1();

	  else if(choose_adc)  config_in_adc2();
	  config_in_adc1();

	  HAL_TIM_Base_Stop(&htim3);
	  HAL_TIM_Base_DeInit(&htim3);

	  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	  TIM_MasterConfigTypeDef sMasterConfig = {0};

	  htim3.Instance = TIM3;
	  htim3.Init.Prescaler = 0;
	  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	  htim3.Init.Period = 7199;
	  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
	  use_hse = SET;
	  max_cnt_adc_1ppt = 20;
	 // HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
	 // HAL_TIM_Base_Start(&htim3);	//Запуск в прерывании

}

void hse_50_kHz(void)
{
	  if(!choose_adc) config_in_adc1();

	  else if(choose_adc)  config_in_adc2();
	  else return;

	  HAL_TIM_Base_Stop(&htim3);
	  HAL_TIM_Base_DeInit(&htim3);
	  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	  TIM_MasterConfigTypeDef sMasterConfig = {0};

	  htim3.Instance = TIM3;
	  htim3.Init.Prescaler = 0;
	  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	  htim3.Init.Period = 1439;
	  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
	  use_hse = SET;
	  max_cnt_adc_1ppt = 100;
	  //HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
	  //HAL_TIM_Base_Start(&htim3);

}

/* Останавливаем АЦП1, если оно было запущено
 * и конфигурируем для запуска от таймера*/
void config_in_adc1(void)
{
	HAL_ADC_Stop_DMA(hadc);
	//HAL_ADCEx_MultiModeStop_DMA(&hadc1);
	//HAL_ADC_DeInit(&hadc2);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_ADC_DeInit(hadc);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_Delay(10);
	ADC_ChannelConfTypeDef sConfig = {0};
	hadc->Instance = ADC1;
	hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
		 if (HAL_ADC_Init(hadc) != HAL_OK)
		 {
			Error_Handler();
		 }

	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
		if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
		{
		   Error_Handler();
		}
}

/* Останавливаем АЦП2, если оно было запущено
 * и конфигурируем для запуска от таймера*/
void config_in_adc2(void)
{
	HAL_ADC_Stop_DMA(hadc);
	//HAL_ADCEx_MultiModeStop_DMA(&hadc1);
	//HAL_ADC_DeInit(&hadc2);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_ADC_DeInit(hadc);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_Delay(10);
	ADC_ChannelConfTypeDef sConfig = {0};
	hadc->Instance = ADC1;
	hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
		 if (HAL_ADC_Init(hadc) != HAL_OK)
		 {
			Error_Handler();
		 }

	sConfig.Channel = ADC_CHANNEL_3;  //CHOOSE CHANNEL
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
		if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
		{
		   Error_Handler();
		}
}
/* Запуск АЦП от внешнего сигнала, он находится на ножке PB11 */
void config_out_adc1(void)
{
	HAL_ADC_Stop_DMA(hadc);
	//HAL_ADCEx_MultiModeStop_DMA(&hadc1);
	//HAL_ADC_DeInit(&hadc2);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_ADC_DeInit(hadc);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_Delay(10);
	ADC_ChannelConfTypeDef sConfig = {0};
	hadc->Instance = ADC1;
	hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_EXT_IT11;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
		 if (HAL_ADC_Init(hadc) != HAL_OK)
		 {
			Error_Handler();
		 }

	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
		if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
		{
		   Error_Handler();
		}
		use_hse = RESET;
	HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
	flag_adc_stop = SET;
	//HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
}

void config_out_adc2(void)
{
	HAL_ADC_Stop_DMA(hadc);
	//HAL_ADCEx_MultiModeStop_DMA(&hadc1);
	//HAL_ADC_DeInit(&hadc2);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_ADC_DeInit(hadc);  // вырубим АЦП, чтобы поменять конфигурацию
	HAL_Delay(10);
	ADC_ChannelConfTypeDef sConfig = {0};
	hadc->Instance = ADC1;
	hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_EXT_IT11;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
		 if (HAL_ADC_Init(hadc) != HAL_OK)
		 {
			Error_Handler();
		 }

	sConfig.Channel = ADC_CHANNEL_3;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
		if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
		{
		   Error_Handler();
		}
		use_hse = RESET;
	HAL_NVIC_EnableIRQ(EXTI1_IRQn); // Включим прерывание, по которому будем получать 1pps
	flag_adc_stop = SET;
	//HAL_ADC_Start_DMA(hadc, (uint32_t*)&rx_buff_ADC_DM, BUFSIZE_ADC);
}


void reboot(void)
{
	  __disable_irq ();
	 if(HAL_TIM_Base_Stop(&htim3)!= HAL_OK) return;
	 if(HAL_ADC_Stop_DMA(hadc)!= HAL_OK) return ;
	 HAL_TIM_Base_DeInit(&htim3);
	 HAL_ADC_DeInit(hadc);



	 //memset(rx_buff_ADC_DM, 0 , BUFSIZE_ADC);
	 flag_dma_send = 0;

	 //flag_adc_stop = SET;
     FLAG_TCP = RESET;
     __enable_irq ();
     HAL_NVIC_DisableIRQ(EXTI1_IRQn); // Выключим прерывание, по которому будем получать 1pps
     flag_adc_stop = SET;
}


void start_multimode(uint32_t *rx_buff)
{
	 //HAL_ADC_Start(&hadc2);
	 // HAL_ADCEx_MultiModeStart_DMA(&hadc1, rx_buff, 400);
}




/*
 * По АЦП прерывание сделать, либо запускать по таймеру, там 100кГц отсчетов
 * Номер сек, именно номер приходит по UART на STM32
 * ПО фронту запускать отсчеты
 * пакет формируем: 32бит Номер сек (из нее вычитать 1с, т.к. отсчет уже произошел)
 * след биты - сами отсчеты 16 бит и данные (пакет max 1460 байт: 1460-32-16 =1412)
 *
 */
