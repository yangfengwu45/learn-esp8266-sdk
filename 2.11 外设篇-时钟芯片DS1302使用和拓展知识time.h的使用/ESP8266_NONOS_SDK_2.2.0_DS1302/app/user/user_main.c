/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_devicefind.h"
#include "user_webserver.h"
#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif
#include "driver/uart.h" //包含uart.h
#include "driver/BufferManage.h"
#include "driver/mDNS.h"

#include "user_tcpclient.h"

#include <time.h>
#include "driver/DS1302.h"


/*******串口接收缓存********/
#define UartReadbuffLen 2048
#define UartManagebuffLen 60
u8  UartReadbuff[UartReadbuffLen];//缓存串口接收的每一条数据
u32 UartManagebuff[UartManagebuffLen];//最大管理的数据条数

u8  UartReadbuffCopy[UartReadbuffLen];//提取缓存数据


os_timer_t os_timer_one;//定义软件定时器结构体变量

uint32 priv_param_start_sec;
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
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            priv_param_start_sec = 0x3C;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            priv_param_start_sec = 0x7C;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
            rf_cal_sec = 512 - 5;
            priv_param_start_sec = 0x7C;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            priv_param_start_sec = 0xFC;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
            rf_cal_sec = 1024 - 5;
            priv_param_start_sec = 0x7C;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            priv_param_start_sec = 0xFC;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            priv_param_start_sec = 0xFC;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            priv_param_start_sec = 0xFC;
            break;
        default:
            rf_cal_sec = 0;
            priv_param_start_sec = 0;
            break;
    }

    return rf_cal_sec;
}
void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
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
int timercnt=0;
int timercnt1=0;
void os_timer_one_function(void *parg)
{
	struct ds1302struct lcTime;

	timercnt++;
	if(timercnt>=1000){//1S
		timercnt=0;

		ds1302_read_time(&lcTime,1);//获取时钟的时间
		//打印 年 月 日 时 分 秒
		os_printf("\nyear=%d,mon=%d,mday=%d,hour=%d,min=%d,sec=%d\n",
				lcTime.tm_year,
				lcTime.tm_mon,
				lcTime.tm_mday,
				lcTime.tm_hour,
				lcTime.tm_min,
				lcTime.tm_sec
				);
	}

	BufferManageRead(&buff_manage,UartReadbuffCopy,&buff_manage.ReadLen);/*取出缓存的数据*/
	if(buff_manage.ReadLen>0){/*缓存取出来数据*/
		user_tcp_send_data(UartReadbuffCopy,buff_manage.ReadLen);
	}
}


void WifiConnectCallback(uint8_t status)
{
	if(status == STATION_GOT_IP){
		os_printf("\nConnect AP Success\n");
	}
	else {
		os_printf("\nDisConnect AP\n");
	}
}



/**
* @brief   系统初始化完成
* @param   None
* @retval  None
* @warning None
* @example
**/
void ICACHE_FLASH_ATTR
system_init_done(void)
{
	struct ds1302struct lcTime;

	ds1302_gpio_init();//初始化引脚

	lcTime.tm_year = 20;//年(芯片只能存储两位 00-99,具体看手册)
	lcTime.tm_mon = 12;//月
	lcTime.tm_mday = 23;//日
	lcTime.tm_hour = 22;//时
	lcTime.tm_min = 55;//分
	lcTime.tm_sec = 5;//秒
	ds1302_set_time(&lcTime,1);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	BufferManageCreate(&buff_manage, UartReadbuff, UartReadbuffLen, UartManagebuff, UartManagebuffLen*4);//创建缓存
	uart_init_2(BIT_RATE_115200,BIT_RATE_115200);

	//配置定时器
	os_timer_setfn(&os_timer_one,os_timer_one_function,NULL);//os_timer_one:定时器结构体变量    os_timer_one_function:回调函数    yang:传给回调函数的参数
	//使能定时器
	os_timer_arm(&os_timer_one,1,1);//os_timer_one:定时器变量        1:1ms进一次    1:循环

	system_init_done_cb(system_init_done);

	/*设置连接的路由器*/
//	WIFI_Connect("QQQQQ","11223344",WifiConnectCallback);
//
//	user_tcp_init("192.168.0.100",8888);//初始化连接的服务器信息
//	user_tcp_connect();//启动连接
}

