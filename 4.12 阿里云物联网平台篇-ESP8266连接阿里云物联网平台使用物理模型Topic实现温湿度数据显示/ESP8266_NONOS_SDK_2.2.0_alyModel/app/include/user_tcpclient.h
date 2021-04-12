#ifndef __USER_TCPCLIENT_H__
#define __USER_TCPCLIENT_H__
#include "ip_addr.h"
#include "espconn.h"

/******
 * 建立tcp链接tcp服务器相关IP和端口号。在user_interface.c 22
 */
#define SSID "test"
#define PASSWORD "wifiwifi"


void user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);
void user_tcp_sent_cb(void *arg);
void user_tcp_discon_cb(void *arg);
void user_send_data(struct espconn *pespconn);
void user_tcp_connect_cb(void *arg);
void user_tcp_recon_cb(void *arg, sint8 err);
void user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
void user_dns_check_cb(void *arg);
void user_check_ip(void);
void user_set_station_config(void);
void tcpuser_init(void);


#endif

