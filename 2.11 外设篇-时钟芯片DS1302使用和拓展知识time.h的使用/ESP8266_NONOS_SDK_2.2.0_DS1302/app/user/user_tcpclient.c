#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"
#include "user_esp_platform.h"
#include "user_tcpclient.h"
#include "driver/uart.h" //包含uart.h

LOCAL int remote_port;
LOCAL char server_ip[4]={0,0,0,0};
LOCAL char *server_domain;

LOCAL char DnsEn=0,DnsSuccessFlag=0;
LOCAL char ConnectState=0;

LOCAL os_timer_t test_timer;
LOCAL os_timer_t acto_timer;



LOCAL struct espconn user_tcp_conn;
LOCAL struct espconn *user_tcp_send;
LOCAL ip_addr_t tcp_server_ip;


/******************************************************************************
 * FunctionName : user_tcp_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
 void ICACHE_FLASH_ATTR
user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
   //received some data from tcp connection

    os_printf("Received data string: %s \r\n", pusrdata);
}
/******************************************************************************
 * FunctionName : user_tcp_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
 void ICACHE_FLASH_ATTR
user_tcp_sent_cb(void *arg)
{
	os_printf("Sent callback: data sent successfully.\r\n");
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
 void ICACHE_FLASH_ATTR
user_tcp_discon_cb(void *arg)
{
	user_tcp_send = NULL;
	ConnectState = 0;
	os_printf("Disconnected from server.\r\n");
}
/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
 void ICACHE_FLASH_ATTR
user_tcp_send_data(uint8 *psent, uint16 length)
{
	 if(ConnectState==1 && user_tcp_send!=NULL)
	 {
		 espconn_send(user_tcp_send, psent, length);
	 }
}

/******************************************************************************
 * FunctionName : user_tcp_connect_cb
 * Description  : A new incoming tcp connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
 void ICACHE_FLASH_ATTR
user_tcp_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    user_tcp_send = pespconn;
    ConnectState = 1;
    os_printf("Connected to server...\r\n");

    espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
    espconn_regist_sentcb(pespconn, user_tcp_sent_cb);
    espconn_regist_disconcb(pespconn, user_tcp_discon_cb);
}

/******************************************************************************
 * FunctionName : user_tcp_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_tcp_recon_cb(void *arg, sint8 err)
{
   //error occured , tcp connection broke. user can try to reconnect here.
	user_tcp_send = NULL;
	ConnectState = 0;
    os_printf("Reconnect callback called, error code: %d !!! \r\n",err);
}


/******************************************************************************
 * FunctionName : user_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR
user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

	if (ipaddr == NULL)
	{
		os_printf("user_dns_found NULL \r\n");
		return;
	}

   //dns got ip
    os_printf("user_dns_found %d.%d.%d.%d \r\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

	if (tcp_server_ip.addr == 0 && ipaddr->addr != 0)
	{
		// dns succeed, create tcp connection
		os_timer_disarm(&test_timer);
		tcp_server_ip.addr = ipaddr->addr;
		os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns

		pespconn->proto.tcp->remote_port = remote_port; // remote port of tcp server
		pespconn->proto.tcp->local_port = espconn_port(); //local port of ESP8266
		DnsSuccessFlag= 1;
	}
}
/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR
user_dns_check_cb(void *arg)
{
    struct espconn *pespconn = arg;
    espconn_gethostbyname(pespconn, server_domain, &tcp_server_ip, user_dns_found); // recall DNS function
}


void ICACHE_FLASH_ATTR
acto_timer_cb(void *arg)
{
    struct espconn *pespconn = arg;
    if(ConnectState ==0)
    {
    	if(DnsEn)
		{
    		if(DnsSuccessFlag)
    		{
				espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
				espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
				espconn_connect(&user_tcp_conn);
    		}
		}
		else
		{
			espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
			espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
			espconn_connect(&user_tcp_conn);
		}
    }
}


/******************************************************************************
 * FunctionName : user_tcp_connect
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_tcp_connect()
{
	if(DnsEn)
	{
		if(DnsSuccessFlag)
		{
			espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
			espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
			espconn_connect(&user_tcp_conn);
		}
	}
	else
	{
		espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
		espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
		espconn_connect(&user_tcp_conn);
	}
	os_timer_disarm(&acto_timer);
	os_timer_setfn(&acto_timer, (os_timer_func_t *)acto_timer_cb, &user_tcp_conn);
	os_timer_arm(&acto_timer, 3000, 1);
}

void ICACHE_FLASH_ATTR
user_tcp_init(char *user_tcp_server_ip,int user_remote_port)
{
	DnsSuccessFlag=0;
	user_tcp_conn.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	user_tcp_conn.type = ESPCONN_TCP;
	user_tcp_conn.state = ESPCONN_NONE;

	remote_port = user_remote_port;

	if(!UTILS_StrToIP(user_tcp_server_ip,server_ip)){
		server_domain = user_tcp_server_ip;
		DnsEn = 1;//可能是域名
	}

	if(DnsEn == 1)
	{
		tcp_server_ip.addr = 0;
		espconn_gethostbyname(&user_tcp_conn, server_domain, &tcp_server_ip, user_dns_found); // DNS function
		os_timer_setfn(&test_timer, (os_timer_func_t *)user_dns_check_cb, &user_tcp_conn);
		os_timer_arm(&test_timer, 1000, 1);
	}
	else
	{
		os_memcpy(user_tcp_conn.proto.tcp->remote_ip,server_ip,4);
		user_tcp_conn.proto.tcp->remote_port =remote_port;  // remote port
		user_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266
	}
}


uint8 UTILS_StrToIP(const char* str, void *ip)
{
	    /* The count of the number of bytes processed. */
	    int i;
	    /* A pointer to the next digit to process. */
	    const char * start;

	    start = str;
	    for (i = 0; i < 4; i++) {
	        /* The digit being processed. */
	        char c;
	        /* The value of this byte. */
	        int n = 0;
	        while (1) {
	            c = * start;
	            start++;
	            if (c >= '0' && c <= '9') {
	                n *= 10;
	                n += c - '0';
	            }
	            /* We insist on stopping at "." if we are still parsing
	               the first, second, or third numbers. If we have reached
	               the end of the numbers, we will allow any character. */
	            else if ((i < 3 && c == '.') || i == 3) {
	                break;
	            }
	            else {
	                return 0;
	            }
	        }
	        if (n >= 256) {
	            return 0;
	        }
	        ((uint8_t*)ip)[i] = n;
	    }
	    return 1;

}

