/*
 * mDNS.c
 *
 *  Created on: 2020Äê7ÔÂ16ÈÕ
 *      Author: yang
 */
#include "driver/mDNS.h"
#include "user_interface.h"
#include "espconn.h"

struct mdns_info info;
void user_mdns_config(void)
{
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	info.host_name = "espressif";
	info.ipAddr = ipconfig.ip.addr; //ESP8266 station IP
	info.server_name = "http";
	info.server_port = 80;
	info.txt_data[0] = "version = now";
	info.txt_data[1] = "user1 = data1";
	info.txt_data[2] = "user2 = data2";
	espconn_mdns_init(&info);
}

