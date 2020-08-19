/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "gpio.h"
#include "esp_timer.h"
#include "hw_timer.h"
#include "uart.h"
#include "pwm.h"

#include "spi_interface.h"




SpiAttr spiConfig;//配置SPI
SpiData SpiSend;//配置SPI发送的数据



extern char Usart0ReadBuff[Usart0ReadLen];//接收数据缓存
extern u32  Usart0ReadCnt;//串口接收的数据个数
extern u32  Usart0ReadCntCopy;//用于拷贝串口接收的数据个数
extern u32  Usart0IdleCnt;//空闲时间累加变量



/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/**
* @brief   硬件定时器中断回调函数
* @param   None
* @param   None
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
u32 cnt = 0;
u8 temp[4] = {0x01,0x02,0x03,0x04};
void hw_test_timer_cb(void)
{
    cnt++;
    if(cnt>1000)//1S
    {
        cnt=0;
        SpiSend.cmd = 0x55;
        SpiSend.cmdLen = 1;
		SpiSend.addr= (uint32_t *)(&temp);//设置发送的数据
		SpiSend.addrLen=4;//发送的数据个数(字节为单位)
		SPIMasterSendData(SpiNum_HSPI,&SpiSend);//发送数据
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	uart_init_new();


	/*配置SPI GPIO 口*/
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_HSPIQ_MISO);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPID_MOSI);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_CLK);

	/*配置SPI 模式*/
	spiConfig.mode = SpiMode_Master;//主机
	spiConfig.speed = SpiSpeed_20MHz;//时钟频率
	spiConfig.subMode = SpiSubMode_0;//模式0
	spiConfig.bitOrder = SpiBitOrder_MSBFirst;//先传输高位
	/*初始化数据*/
	SpiSend.addr=0;
	SpiSend.addrLen=0;
	SpiSend.cmd=0;
	SpiSend.cmdLen=0;
	SPIInit(SpiNum_HSPI,&spiConfig);//初始化SPI


    //定时器初始化
    hw_timer_init(1);//1:循环
    //设置定时器回调函数
    hw_timer_set_func(hw_test_timer_cb);//hw_test_timer_cb:硬件定时器中断回调函数
    hw_timer_arm(1000);//1000:1000us定时进入中断函数
}

