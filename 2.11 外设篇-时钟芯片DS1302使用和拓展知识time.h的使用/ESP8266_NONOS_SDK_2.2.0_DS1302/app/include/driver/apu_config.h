/*
 * apu_config.h
 *
 *  Created on: 2020年4月9日
 *      Author: yang
 */

#ifndef __APU_CONFIG_H__
#define __APU_CONFIG_H__

#define  USSID "wifi_8266_bind" 		//AP无线名称
#define  UPWD "11223344"     			//密码

typedef enum {
	APU_STATUS_GETING_DATA=0,//获取了APP发送的数据
	APU_STATUS_LINK_SSID_PSWD,//开始连接路由器
	APU_STATUS_LINKED,//连接上了路由器
	APU_STATUS_UDPSEND,//模块返回UDP数据,MAC地址和IP地址
	APU_STATUS_LINK_OVER,//结束
} apuconfig_status;


typedef void (*apuconfig_callback_t)(apuconfig_status status, void *pdata);


void apuconfig_start(apuconfig_callback_t cb,uint8 time_out);
void apuconfig_stop();

#endif
