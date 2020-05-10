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


#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"
#include "lwip/tcp.h"


extern char Usart0ReadBuff[Usart0ReadLen];//接收数据缓存
extern u32  Usart0ReadCnt;//串口接收的数据个数
extern u32  Usart0ReadCntCopy;//用于拷贝串口接收的数据个数
extern u32  Usart0IdleCnt;//空闲时间累加变量


/*用于返回继电器的状态*/
u8 RelayOn[4]={0x55,0xaa,0x01,0x01};//继电器吸合
u8 RelayOff[4]={0x55,0xaa,0x01,0x00};//继电器断开


struct tcp_pcb *tcp_pcb_server;//定义一个TCP控制块
#define TcpServerBuffLen 1460
u8 TcpServerBuff[TcpServerBuffLen];//接收缓存
u8 tcp_pcb_server_state = 0;//记录连接状态

/**
* @brief   TCP接收数据
* @param   arg:tcp_arg函数传入的参数
* * @param   p:接收的数据缓存
* @param   err:错误信息
* @param   None
* @retval  None
* @warning None
* @example
**/
static err_t net_tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	struct pbuf *q;
	u32 length = 0,i=0;
	tcp_pcb_server = tpcb;
	tcp_pcb_server_state = 1;
	if (!p || err!=ERR_OK) {
		if(p){
			pbuf_free(p);
		}
		tcp_pcb_server_state = 0;
		tcp_close(tcp_pcb_server);//关闭连接
		return ERR_CLSD;
	}
	//接收TCP数据(固定)
	for(q=p;q!=NULL;q=q->next){
		if(q->len > (TcpServerBuffLen-length))//接收的数据个数大于了数组可以接收的数据个数
			memcpy(TcpServerBuff+length,q->payload,(TcpServerBuffLen-length));//只接收数组可以接收的数据个数
		else
			memcpy(TcpServerBuff+length,q->payload,q->len);//接收TCP所有数据
		length += q->len;
		if(length > TcpServerBuffLen) break;
	}

	if(TcpServerBuff[0] == 0xaa && TcpServerBuff[1] == 0x55){
		if(TcpServerBuff[2] == 0x01){
			if(TcpServerBuff[3] == 0x01){
				GPIO_OUTPUT_SET(5, 1);//设置GPIO5输出高电平
				tcp_write(tcp_pcb_server, RelayOn, 4, TCP_WRITE_FLAG_COPY);//TCP_WRITE_FLAG_COPY:拷贝到发送缓存
				tcp_output(tcp_pcb_server);//立即发送
			}
			else if(TcpServerBuff[3] == 0x00){
				GPIO_OUTPUT_SET(5, 0);//设置GPIO5输出低电平
				tcp_write(tcp_pcb_server, RelayOff, 4, TCP_WRITE_FLAG_COPY);//TCP_WRITE_FLAG_COPY:拷贝到发送缓存
				tcp_output(tcp_pcb_server);//立即发送
			}
		}
	}

	//固定处理
	tcp_recved(tcp_pcb_server, p->tot_len);/*更新接收,告诉底层可以接着缓存数据了*/
	pbuf_free(p);//释放链表
	return ERR_OK;
}


/**
* @brief   TCP链接错误
* @param   arg:tcp_arg函数传入的参数
* @param   err:错误信息
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
static void net_err_cb(void *arg, err_t err) {
	tcp_pcb_server = (struct tcp_pcb*)arg; //tcp_arg传递了该参数
	tcp_pcb_server_state = 0;
    tcp_close(tcp_pcb_server);//关闭连接
    tcp_pcb_server = NULL;//清空
}

/**
* @brief   客户端连接回调
* @param   arg:tcp_arg函数传入的参数
* @param   newpcb:链接的TCP控制块
* @param   err:错误信息
* @param   None
* @retval  None
* @warning None
* @example
**/
static err_t net_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
	tcp_pcb_server = newpcb;//赋值给定义的控制块
	tcp_pcb_server_state = 1;
	tcp_arg(newpcb, newpcb);//传递的arg参数为 tcp_pcb_server
	tcp_err(newpcb, net_err_cb);//错误回调
	tcp_recv(newpcb, net_tcp_recv_cb);//接收数据回调

	printf("客户端连接 \n");
	return ERR_OK;
}



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
void hw_test_timer_cb(void)
{
	if(Usart0ReadCnt!=0){//串口接收到数据
		Usart0IdleCnt++;//空闲时间累加
		if(Usart0IdleCnt>Usart0IdleTime){//累加到期望值(10ms)
			Usart0IdleCnt=0;
			Usart0ReadCntCopy = Usart0ReadCnt;//拷贝接收的数据个数
			Usart0ReadCnt=0;
			/*处理数据
			 * 数据缓存数组:Usart0ReadBuff
			 * 数据长度:Usart0ReadCntCopy
			 * */
			if(tcp_pcb_server_state == 1){//有客户端连接
				tcp_write(tcp_pcb_server, Usart0ReadBuff, Usart0ReadCntCopy, TCP_WRITE_FLAG_COPY);//TCP_WRITE_FLAG_COPY:拷贝到发送缓存
				tcp_output(tcp_pcb_server);//立即发送
			}
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
	err_t err = ERR_OK;//接收返回的错误信息

	uart_init_new();


    /*设置GPIO5为普通引脚*/
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U , FUNC_GPIO5);

	struct tcp_pcb *tcp_pcb1 = tcp_new();//建立一个TCP控制块

	//控制块绑定IP地址和端口号
	err = tcp_bind(tcp_pcb1, IP_ADDR_ANY, 8080);//IP_ADDR_ANY:绑定本模块IP  8080:绑定8080端口
	if (err == ERR_OK) {//没有错误
		struct tcp_pcb *pcb1 = tcp_listen(tcp_pcb1);//启动监听

		if (pcb1) {
			tcp_accept(pcb1, net_accept_cb);//设置监听到客户端连接的回调函数
		}
	}

    //定时器初始化
    hw_timer_init(1);//1:循环
    //设置定时器回调函数
    hw_timer_set_func(hw_test_timer_cb);//hw_test_timer_cb:硬件定时器中断回调函数
    hw_timer_arm(1000);//1000:1000us定时进入中断函数
}

