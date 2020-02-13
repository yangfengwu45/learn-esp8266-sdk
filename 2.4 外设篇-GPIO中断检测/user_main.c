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
* @brief   GPIO0中断回调函数
* @param   None
* @param   None
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
void gpio0_intr_handler()
{
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);//读取GPIO状态寄存器，获取中断信息
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);//清除中断信息
    if(gpio_status & (BIT(0)))//GPIO0产生的中断
    {
    	if(!GPIO_INPUT_GET(0))//GPIO0是低电平	GPIO0确实是下降沿产生了中断
    	{
    		printf("GpioDown\r\n");
    	}
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
    printf("SDK version:%s\n", system_get_sdk_version());
    
	printf("Ai-Thinker Technology Co. Ltd.\r\n%s %s\r\n", __DATE__, __TIME__);


	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U , FUNC_GPIO0);//GPIO0做为普通IO使用
	GPIO_DIS_OUTPUT(0);//0:GPIO0	如果以前设置过GPIO为输出,则调用此函数关闭GPIO0输出
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);//GPIO0上拉输入

	_xt_isr_mask(1<<ETS_GPIO_INUM);    //关闭GPIO中断
	gpio_intr_handler_register(gpio0_intr_handler, NULL);//设置中断函数
	gpio_pin_intr_state_set(0, GPIO_PIN_INTR_NEGEDGE);//0:GPIO0		GPIO_PIN_INTR_NEGEDGE:下降沿触发
	_xt_isr_unmask(1 << ETS_GPIO_INUM); //使能GPIO中断
}

