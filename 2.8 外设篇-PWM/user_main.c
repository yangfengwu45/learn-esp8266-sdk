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


extern char Usart0ReadBuff[Usart0ReadLen];//接收数据缓存
extern u32  Usart0ReadCnt;//串口接收的数据个数
extern u32  Usart0ReadCntCopy;//用于拷贝串口接收的数据个数
extern u32  Usart0IdleCnt;//空闲时间累加变量

int i;

os_timer_t os_timer_one;//定义一个定时器结构体变量
int LightValue = 1;

                       //引脚寄存器地址                                  复用值(普通IO)  引脚序号
uint32 io_info[1][3] = {PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, 2};
int duty[1]={1023/1000*0};//高电平时间约是0us



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
* @brief   定时器回调函数
* @param   parg:传入的配置os_timer_setfn函数最后的参数
* @param   None
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
void os_timer_one_function(void *parg)
{
	pwm_set_duty(duty[0],0);//设置0通道的PWM时间
	pwm_start();//启动PWM
	//如果LightValue是1,则每次累加5;如果LightValue是-1,则每次减5
	duty[0] = duty[0] + 5*LightValue;
	if(duty[0]>1023){//达到最大值
		duty[0] = 1023;//设置为最大值
		LightValue = -1;//开始减小
	}
	if(duty[0]<0){//达到最小值
		duty[0] = 0;//设置为最小值
		LightValue=1;//开始累加
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
	//1000:周期1000us duty:高电平时间100us   1:就配置了一个管脚,因为数组是[1][3]   io_info:io_info数组
	pwm_init(1000, duty, 1, io_info);
	pwm_start();//启动PWM

    //配置定时器
    os_timer_setfn(&os_timer_one,os_timer_one_function,"yang");//os_timer_one:定时器结构体变量    os_timer_one_function:回调函数    yang:传给回调函数的参数
    //使能定时器
    //每隔10ms改变LED灯的亮度(人眼看不出来小于20ms的瞬间变化,会看成连续的)
    os_timer_arm(&os_timer_one,10,1);//os_timer_one:定时器变量        10:10ms进一次    1:循环
}

