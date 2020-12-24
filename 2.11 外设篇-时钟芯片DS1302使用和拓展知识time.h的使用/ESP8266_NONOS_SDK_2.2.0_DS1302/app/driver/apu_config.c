/*
 * apu_config.c
 *
 *  Created on: 2020年4月9日
 *      Author: yang
 */

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "driver/apu_config.h"
#include "user_json.h"


/*存储接收到的路由器名称和密码*/
char RecvSSID[64];
char RecvPWD[64];
char RecvWifiInfoFlag=0;

uint8_t ThisMAC[22];//存储模块的MAC地址
uint8_t ThisIP[22];//存储模块连接路由器分得的IP地址

uint8_t UDPSendData[100];
uint8_t UDPSendDataLen=0;
uint8_t UDPSendDataCnt=0;//发送数据的次数,超过3次默认发送成功
uint8_t UDPSendDataTime=0;

uint8_t TimerOutCnt=0;//超时计数
uint8_t TimerOutValue=0;//超时时间

struct softap_config soft_ap_Config;	//AP模式配置     定义AP参数结构体
struct espconn espconn_udp;//UDP连接配置的结构体



static ETSTimer WiFiLinker;//定时检测连接路由器
static ETSTimer MainTime;//MainTime


apuconfig_callback_t wifiCb = NULL;



static void ICACHE_FLASH_ATTR main_time(void *arg)
{
	if(TimerOutCnt<TimerOutValue){
		TimerOutCnt++;

		if(UDPSendDataCnt>=1){//已经向APP发送了数据
			UDPSendDataTime++;
		}

		if(UDPSendDataCnt>=3 || UDPSendDataTime>=3){//发送数据的次数,超过3次默认发送成功(配网绑定结束),发送了一次以后超时
			os_timer_disarm(&MainTime);//停止定时器
			os_timer_disarm(&WiFiLinker);//停止定时器
			wifi_set_opmode(STATION_MODE);//station模式
			UDPSendDataCnt=0;
			UDPSendDataTime=0;
			TimerOutCnt = 0;
			if(wifiCb){//配网结束
				wifiCb(APU_STATUS_LINK_OVER,NULL);
			}
		}
	}
	else{//超时
		os_timer_disarm(&MainTime);//停止定时器
		os_timer_disarm(&WiFiLinker);//停止定时器
		wifi_set_opmode(STATION_MODE);//station模式
		UDPSendDataCnt=0;
		TimerOutCnt = 0;
		if(wifiCb){//配网结束
			wifiCb(APU_STATUS_LINK_OVER,NULL);
		}
	}

}


//定时检测连接路由器的状态
static uint8_t wifiStatus = STATION_IDLE;
static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	struct ip_info ipConfig;

	wifi_get_ip_info(STATION_IF, &ipConfig);
	wifiStatus = wifi_station_get_connect_status();
	if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)//连接成功
	{
		os_bzero(ThisIP, 22);
//		os_printf("self:"IPSTR"\n",IP2STR(&ipConfig.ip));

		os_sprintf(ThisIP,"%d.%d.%d.%d",IP2STR(&ipConfig.ip));

//		os_printf("\n ThisIP:%s\n",ThisIP);
		os_timer_disarm(&WiFiLinker);//停止轮训

		if(wifiCb){//连接上了路由器
			wifiCb(APU_STATUS_LINKED,ThisIP);
		}
	}
	else
	{
		if(wifiStatus == STATION_WRONG_PASSWORD)
		{
//			os_printf("STATION_WRONG_PASSWORD\r\n");
			RecvWifiInfoFlag=0;
		}
		else if(wifiStatus == STATION_NO_AP_FOUND)
		{
//			os_printf("STATION_NO_AP_FOUND\r\n");
			RecvWifiInfoFlag=0;
		}
		else if(wifiStatus == STATION_CONNECT_FAIL)
		{
//			os_printf("STATION_CONNECT_FAIL\r\n");
			RecvWifiInfoFlag=0;
		}
		else
		{
//			os_printf("STATION_IDLE\r\n");
		}
	}
}


void ConnectWifi(uint8_t* ssid, uint8_t* pass){
	struct station_config stationConf;
	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", ssid);
	os_sprintf(stationConf.password, "%s", pass);

//	os_printf("stationConf.ssid : %s \r\n",stationConf.ssid);
//	os_printf("stationConf.pwd  : %s \r\n",stationConf.password);

	if(wifiCb){
		wifiCb(APU_STATUS_LINK_SSID_PSWD,&stationConf);
	}

	wifi_station_set_config(&stationConf);

	//定时器轮训检测wifi连接状态
	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 200, 1);

	//wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();//连接wifi
}


LOCAL int ICACHE_FLASH_ATTR
msg_set(struct jsontree_context *js_ctx,struct jsonparse_state *parser)
{
	int type;
	while( (type = jsonparse_next(parser)) != 0)
	{
		if(jsonparse_strcmp_value(parser,"ssid") == 0)
		{
			os_bzero(RecvSSID, 64);
			jsonparse_next(parser);
			jsonparse_next(parser);
			jsonparse_copy_value(parser, RecvSSID, sizeof(RecvSSID));
//			os_printf("ssid : %s \r\n",RecvSSID);
		}
		else if(jsonparse_strcmp_value(parser,"pwd") == 0){
			os_bzero(RecvPWD, 64);
			jsonparse_next(parser);
			jsonparse_next(parser);
			jsonparse_copy_value(parser, RecvPWD, sizeof(RecvPWD));
//			os_printf("pwd : %s \r\n",RecvPWD);

			if(RecvWifiInfoFlag==0){
				RecvWifiInfoFlag = 1;

				ConnectWifi(RecvSSID,RecvPWD);//连接路由器
			}
		}
	}
	return 0;
}


struct jsontree_callback msg_callback =
JSONTREE_CALLBACK(NULL,msg_set);
JSONTREE_OBJECT(msg_tree,JSONTREE_PAIR("ssid",&msg_callback),JSONTREE_PAIR("pwd",&msg_callback));



//回调函数    等待接收客户端的消息. 消息格式{"ssid":"qqqqq","pwd":"11223344"}
void ICACHE_FLASH_ATTR udpclient_recv(void *arg, char *pdata, unsigned short len)
{
	struct espconn *T_arg=arg;
	remot_info *P_port_info=NULL;

	//接收到微信手机发出的信息，手机登录到ESP8266的AP上，手机的IP地址比如：192.168.4.2
//	os_printf("Receive message:%s\n",pdata);							//{"ssid":"qqqqq","pwd":"11223344"}真正的无线路由器


	if(wifiCb){
		wifiCb(APU_STATUS_GETING_DATA,pdata);
	}

	//解析json数据
	struct jsontree_context js;
	jsontree_setup(&js,(struct jsontree_value *)&msg_tree,json_putchar);
	json_parse(&js,pdata);

	if(espconn_get_connection_info(T_arg,&P_port_info,0)==ESPCONN_OK)	//获取发送这次UDP数据的远端主机消息
	{
		T_arg->proto.udp->remote_port=P_port_info->remote_port;
		T_arg->proto.udp->remote_ip[0]=P_port_info->remote_ip[0];
		T_arg->proto.udp->remote_ip[1]=P_port_info->remote_ip[1];
		T_arg->proto.udp->remote_ip[2]=P_port_info->remote_ip[2];
		T_arg->proto.udp->remote_ip[3]=P_port_info->remote_ip[3];

		if(ThisIP[0] !=0){
			UDPSendDataLen = os_sprintf(UDPSendData,"{\"mac\":\"%s\",\"ip\":\"%s\"}",ThisMAC,ThisIP);
//			os_printf("\nUDPSendData:%s\n",UDPSendData);
			espconn_send(T_arg,UDPSendData,UDPSendDataLen);//向对方发送应答
			UDPSendDataCnt++;

			if(wifiCb){
				wifiCb(APU_STATUS_UDPSEND,UDPSendData);
			}
		}
	}
}


void InitAPUConfig(){
	uint8_t mac[6];
	wifi_set_opmode(STATIONAP_MODE);//station+ soft-ap模式
	soft_ap_Config.ssid_len = strlen(USSID);						//热点名称长度，与你实际的名称长度一致就好
	memcpy(soft_ap_Config.ssid,USSID,soft_ap_Config.ssid_len);		//实际热点名称设置，可以根据你的需要来
	memcpy(soft_ap_Config.password,UPWD,strlen(UPWD));				//热点密码设置
	soft_ap_Config.authmode = AUTH_WPA2_PSK;						//加密模式
	soft_ap_Config.channel = 1;										//信道，共支持1~13个信道
	soft_ap_Config.max_connection = 4;								//最大连接数量，最大支持四个，默认四个
	wifi_softap_set_config_current(&soft_ap_Config);				//设置 Wi-Fi SoftAP 接口配置，不保存到 Flash

	os_bzero(ThisMAC, 22);
	wifi_get_macaddr(STATION_IF, mac);//获取设备MAC地址
	os_sprintf(ThisMAC, MACSTR, MAC2STR(mac));
	os_printf("\n MAC:%s\n",ThisMAC);
}


void InitUDP(){
	espconn_init();//"espconn.h"   195行
	espconn_udp.type = ESPCONN_UDP;     //创建
	espconn_udp.state = ESPCONN_NONE;   //一开始的状态
	espconn_udp.proto.udp = (esp_udp *)os_malloc(sizeof(esp_udp));
	espconn_udp.proto.udp->local_port = 5556;//监听的端口号

    //注册UDP数据包接收回调
    espconn_regist_recvcb(&espconn_udp, udpclient_recv);   //UDP接收数据回调
    espconn_create(&espconn_udp);							//建立UDP传输
}


/**
* @brief   启动apuconfig
* @param   time_out:超时时间 S   默认60S
* @param   cb:配网回调函数
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
void apuconfig_start(apuconfig_callback_t cb,uint8 time_out){
	os_bzero(ThisIP, 22);
	TimerOutCnt=0;
	UDPSendDataCnt=0;
	UDPSendDataTime=0;
	//启动需要清零的变量

	RecvWifiInfoFlag = 0;
	InitAPUConfig();
	InitUDP();

	wifiCb = cb;
	if(time_out==0){
		TimerOutValue=60;
	}else{
		TimerOutValue = time_out;
	}
	os_timer_disarm(&MainTime);
	os_timer_setfn(&MainTime, (os_timer_func_t *)main_time, NULL);
	os_timer_arm(&MainTime, 1000, 1);

}


/**
* @brief   停止apuconfig
* @param   None
* @param   None
* @param   None
* @param   None
* @retval  None
* @warning None
* @example
**/
void  apuconfig_stop(void){
	os_timer_disarm(&MainTime);//停止定时器
	os_timer_disarm(&WiFiLinker);//停止定时器
	wifi_set_opmode(STATION_MODE);//station模式
}






