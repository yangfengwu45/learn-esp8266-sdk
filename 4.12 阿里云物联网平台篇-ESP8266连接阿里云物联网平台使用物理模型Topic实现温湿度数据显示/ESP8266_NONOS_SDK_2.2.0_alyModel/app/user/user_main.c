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
#include "user_tcpclient.h"
#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

#include "driver/uart.h" //包含uart.h
#include  "espconn.h"
#include  "mem.h"
#include "driver/wifi.h"
#include "driver/mqtt.h"
#include "smartconfig.h"

#include "gpio.h"
#include "eagle_soc.h"

#include "driver/i2c_master.h"
#include "driver/oled.h"
#include "driver/dht11.h"

#include "driver/aly_hmac.h"

unsigned char MainBuffer[1460];//缓存数据,全局通用
u32  MainLen=0;      //全局通用变量


//修改在云平台获取的设备信息
char ProductKey[50]="a1m7er1nJbQ";//替换自己的 ProductKey
char DeviceName[50]="Mqtt";//替换自己的 DeviceName
char DeviceSecret[50]="7GUrQwgDUcXWV3EIuLwdEvmRPWcl7VsU";//替换自己的 DeviceSecret
char Region[50]="cn-shanghai";//地区,根据自己的修改
char ClientID[50]="112233445566";//修改自己设置的

//连接MQTT
unsigned char IP[100]="";//IP地址/域名
unsigned int  Port = 1883;//端口号
unsigned char MQTTid[100] = "";//ClientID
unsigned char MQTTUserName[100] = "";//用户名
unsigned char MQTTPassWord[100] = "";//密码
unsigned char MQTTkeepAlive = 30;//心跳包时间
/*阿里云提供的自定义消息通信的主题*/
unsigned char MQTTPublishTopicCustom[100]="";//存储MQTT发布的主题
unsigned char MQTTSubscribeTopicCustom[100]="";//存储MQTT订阅的主题
/*阿里云提供的标准格式消息通信的主题(物模型)*/
unsigned char MQTTPublishTopicModel[100]="";//存储MQTT发布的主题
unsigned char MQTTSubscribeTopicModel[100]="";//存储MQTT订阅的主题

MQTT_Client mqttClient;
MQTT_Client *client;//获取链接的MQTT_Client


os_timer_t os_timer_one;//定义软件定时器结构体变量

extern u8  Usart0ReadBuff[Usart0ReadLen];//接收数据的数组
extern u32 Usart0ReadCnt;//串口1接收到的数据个数
extern u32 Usart0IdelCnt;
extern u32 Usart0ReadCntCopy;//串口1接收到的数据个数拷贝

char RelayState = 0;//记录继电器状态
u32 RendTHCnt = 0;//采集DHT11延时

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
    uint32 priv_param_start_sec;

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

void os_timer_one_function(void *parg)
{
	MQTT_Connect(&mqttClient);
}

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){

	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

//连接上MQTT
void mqttConnectedCb(uint32_t *args){
	client = (MQTT_Client*)args;
	os_printf("MQTT: Connected\r\n");
	MQTT_Subscribe(client, MQTTSubscribeTopicCustom, 0);//订阅自定义消息通信的主题
	MQTT_Subscribe(client, MQTTSubscribeTopicModel, 0);//订阅主题
	os_timer_disarm(&os_timer_one);//停止定时连接MQTT
}

//连接断开
void mqttDisconnectedCb(uint32_t *args){
	client = (MQTT_Client*)args;
	os_printf("MQTT: Disconnected\r\n");
	os_timer_arm(&os_timer_one,3000,1);//配置定时器连接MQTT
}
//发送完消息
void mqttPublishedCb(uint32_t *args){
	client = (MQTT_Client*)args;
	os_printf("MQTT: Published\r\n");
}
//接收到数据
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
	*dataBuf = (char*)os_zalloc(data_len+1);//用来缓存主题和消息

	MQTT_Client* client = (MQTT_Client*)args;
	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;
	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;


	if(os_strstr((char*)dataBuf, "\"data\":\"switch\""))//询问开关
	{
		if(os_strstr((char*)dataBuf, "\"bit\":\"1\""))//第一路开关
		{
			if(os_strstr((char*)dataBuf, "\"status\":\"-1\""))//询问状态
			{
				RelayState = ~RelayState;//改变状态,让检测触发
			}
			else if( os_strstr((char*)dataBuf, "\"status\":\"1\"") )
			{
				GPIO_OUTPUT_SET(5, 1);//设置GPIO5输出高电平
			}
			else if(os_strstr((char*)dataBuf, "\"status\":\"0\""))
			{
				GPIO_OUTPUT_SET(5, 0);//设置GPIO5输出低电平
			}
		}
	}

	os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);//打印接收的消息
	os_free(topicBuf);
	os_free(dataBuf);
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
	if(RelayState != GPIO_INPUT_GET(5))//继电器状态变化,发送继电器状态
	{
		RelayState = GPIO_INPUT_GET(5);

		if(RelayState)
		{
			MainLen = os_sprintf((char*)MainBuffer,"{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"1\"}");//发送的数据
		}
		else
		{
			MainLen = os_sprintf((char*)MainBuffer,"{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"0\"}");//发送的数据
		}
		/*往阿里云提供的自定义消息通信的主题发送消息*/
		if(client != NULL){
			MQTT_Publish(client, MQTTPublishTopicCustom, MainBuffer, MainLen, 0, 0);//发布消息
		}
	}

    if(Usart0ReadCnt!=0){//串口接收到数据
    	Usart0IdelCnt++;//空闲时间累加
        if(Usart0IdelCnt>10){//累加到期望值(10ms)
        	Usart0IdelCnt=0;
            Usart0ReadCntCopy = Usart0ReadCnt;//拷贝接收的数据个数
            Usart0ReadCnt=0;
            /*处理数据
             * 数据缓存数组:Usart0ReadBuff
             * 数据长度:Usart0ReadCntCopy
             * */
        }
    }

	RendTHCnt++;
	if(RendTHCnt>=2000){
		RendTHCnt=0;
		DHT11_Read_Data();
		OLED_ShowNum(55,3, DHT11Data[2],2,16);//
		OLED_ShowNum(55,5, DHT11Data[0],2,16);//

		/*往阿里云提供的自定义消息通信的主题发送消息*/
		MainLen = os_sprintf((char*)MainBuffer,"{\"data\":\"TH\",\"bit\":1,\"temperature\":%d,\"humidity\":%d}",DHT11Data[2],DHT11Data[0]);
		if(client != NULL)
		{
			/*往阿里云提供的自定义消息通信的主题发送消息*/
			MQTT_Publish(client, MQTTPublishTopicCustom, MainBuffer, MainLen, 0, 0);//发布消息

			/*往阿里云提供的标准格式消息通信的主题(物模型)发送消息*/
						MainLen = os_sprintf((char*)MainBuffer,"{\"method\":\"thing.event.property.post\",\"id\":1111,\
			\"params\":{\"temp\":%d,\"humi\":%d},\"version\":\"1.0\"}",DHT11Data[2],DHT11Data[0]);
						MQTT_Publish(client, MQTTPublishTopicModel, MainBuffer, MainLen, 0, 1);//发布消息
		}
	}
}


/**
* @brief   初始化MQTT
* @param   None
* @retval  None
* @warning None
* @example
**/
void InitMQTT(void){
	MQTT_InitConnection(&mqttClient, IP, Port, 0);//MQTT服务器IP地址,端口号,是否SSL
	MQTT_InitClient(&mqttClient, MQTTid, MQTTUserName, MQTTPassWord, MQTTkeepAlive, 1);//ClientID,用户名,密码,心跳包时间,清除连接信息
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);//设置连接回调
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);//设置断开回调
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);//设置发送完消息回调
	MQTT_OnData(&mqttClient, mqttDataCb);//接收数据回调

	//配置定时器连接MQTT
	os_timer_setfn(&os_timer_one,os_timer_one_function,NULL);
	//使能定时器
	os_timer_arm(&os_timer_one,3000,1);
}

/*初始化MQTT参数*/
void ICACHE_FLASH_ATTR
aly_config(void){
	//组合IP地址
	os_memset(IP,0,sizeof(IP));
	os_sprintf((char *)IP,"%s.iot-as-mqtt.%s.aliyuncs.com",ProductKey,Region);
	os_printf("\nIP:%s\n",IP);

	//组合设备的ID
	os_memset(MQTTid,0,sizeof(MQTTid));
	os_sprintf(MQTTid,"%s|securemode=3,signmethod=hmacsha1|",ClientID);
	os_printf("\nMQTTid:%s\n",MQTTid);

	//组合用户名
	os_memset(MQTTUserName,0,sizeof(MQTTUserName));
	os_sprintf((char *)MQTTUserName,"%s&%s",DeviceName,ProductKey);
	os_printf("\nMQTTUserName:%s\n",MQTTUserName);

	//组合订阅的主题
	os_memset(MQTTSubscribeTopicCustom,0,sizeof(MQTTSubscribeTopicCustom));
	os_sprintf((char *)MQTTSubscribeTopicCustom,"/%s/%s/user/get",ProductKey,DeviceName);
	//组合发布的主题
	os_memset(MQTTPublishTopicCustom,0,sizeof(MQTTPublishTopicCustom));
	os_sprintf((char *)MQTTPublishTopicCustom,"/%s/%s/user/update",ProductKey,DeviceName);


	//组合订阅的主题
	os_memset(MQTTSubscribeTopicModel,0,sizeof(MQTTSubscribeTopicModel));
	os_sprintf((char *)MQTTSubscribeTopicModel,"/sys/%s/%s/thing/service/property/set",ProductKey,DeviceName);
	os_printf("\nMQTTSubscribeTopic:%s\n",MQTTSubscribeTopicModel);

	//组合发布的主题
	os_memset(MQTTPublishTopicModel,0,sizeof(MQTTPublishTopicModel));
	os_sprintf((char *)MQTTPublishTopicModel,"/sys/%s/%s/thing/event/property/post",ProductKey,DeviceName);
	os_printf("\nMQTTPublishTopic:%s\n",MQTTPublishTopicModel);


	//组合计算密码
	os_sprintf(MainBuffer,"clientId%sdeviceName%sproductKey%s",ClientID,DeviceName,ProductKey);
	aly_hmac_sha1(MainBuffer,strlen(MainBuffer),DeviceSecret,strlen(DeviceSecret),MQTTPassWord);
	os_printf("\nMQTTPassWord:%s\n",MQTTPassWord);
}


/**
* @brief   初始化OLED
* @param   None
* @retval  None
* @warning None
* @example
**/
void ICACHE_FLASH_ATTR
InitOLED(void){
	/*<初始化OLED*/
	i2c_master_gpio_init();//初始化引脚
	OLED_Init();
	OLED_Clear();
	OLED_ShowCHinese(36,0,0);//温
	OLED_ShowCHinese(54,0,1);//湿
	OLED_ShowCHinese(72,0,2);//度
	OLED_ShowString(25,3,"T :",16);
	OLED_ShowString(25,5,"H :",16);
	OLED_ShowCHinese(80,3,3);//℃
	OLED_ShowString(80,5," %",16);
	/*初始化OLED>*/
}


/**
* @brief   初始化GPIO
* @param   None
* @retval  None
* @warning None
* @example
**/
void ICACHE_FLASH_ATTR
InitGPIO(void){
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U , FUNC_GPIO0);//设置GPIO0普通模式
    GPIO_OUTPUT_SET(0, 1);//设置GPIO0输出高电平

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U , FUNC_GPIO2);//设置GPIO2普通模式
    GPIO_OUTPUT_SET(2, 1);//设置GPIO2输出高电平

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U , FUNC_GPIO5);//设置GPIO5普通模式
    GPIO_OUTPUT_SET(5, 0);//设置GPIO5输出低电平
}


/**
* @brief   系统初始化完成
* @param   None
* @retval  None
* @warning None
* @example
**/
void system_init_done(void)
{
	InitGPIO();//初始化GPIO
	InitOLED();//初始化OLED
	aly_config();//初始化MQTT参数
	InitMQTT();//初始化MQTT

    //定时器初始化
	hw_timer_init(0,1);//1:循环
	//设置定时器回调函数
	hw_timer_set_func(hw_test_timer_cb);//hw_test_timer_cb:硬件定时器中断回调函数
	hw_timer_arm(1000);//1000:1000us定时进入中断函数

	WIFI_Connect("QQQQQ", "11223344", wifiConnectCb);//连接路由器
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
	uart_init_2(BIT_RATE_115200,BIT_RATE_115200);
	system_init_done_cb(system_init_done);
}

