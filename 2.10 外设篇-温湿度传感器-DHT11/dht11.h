#ifndef __DHT11_H_
#define __DHT11_H_

#ifndef _DHT11_C_
#define _DHT11_C_ extern
#else
#define _DHT11_C_
#endif
  
#include "c_types.h"

#define OW_MASTER_MUX PERIPHS_IO_MUX_GPIO4_U
#define OW_MASTER_GPIO 4
#define OW_MASTER_FUNC FUNC_GPIO4


#define	DHT11_DQ_IN  GPIO_INPUT_GET(OW_MASTER_GPIO)


_DHT11_C_ u8 DHT11Data[4];//温湿度数据(温度高位,温度低位,湿度高位,湿度低位)

void DHT11_start(void);
void DHT11_Receive(void);     //接收40位的数据


void DHT11_Read_Data(void); 
u8 DHT11_Read_Byte(void);//??????
u8 DHT11_Read_Bit(void);//?????
u8 DHT11_Check(void);//??????DHT11
void DHT11_Rst(void);//??DHT11  



#endif

