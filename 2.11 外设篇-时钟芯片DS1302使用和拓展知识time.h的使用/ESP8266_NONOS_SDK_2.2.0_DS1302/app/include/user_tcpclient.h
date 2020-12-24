#ifndef __USER_TCPCLIENT_H__
#define __USER_TCPCLIENT_H__
#include "ip_addr.h"
#include "espconn.h"

/******
 * 建立tcp链接tcp服务器相关IP和端口号。在user_interface.c 22
 */
#define SSID "QQQQQ"
#define PASSWORD "11223344"


void user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);
void user_tcp_sent_cb(void *arg);
void user_tcp_discon_cb(void *arg);
void user_tcp_send_data(uint8 *psent, uint16 length);
void user_tcp_connect_cb(void *arg);
void user_tcp_recon_cb(void *arg, sint8 err);
void user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
void user_dns_check_cb(void *arg);
void user_tcp_connect();
void user_tcp_init(char *user_tcp_server_ip,int user_remote_port);
uint8 UTILS_StrToIP(const char* str, void *ip);
#endif

