#define _DHT11_C_

#include "eagle_soc.h"
#include "../include/driver/dht11.h"
#include "osapi.h"
#include "gpio.h"

/**
* 介绍:  DHT11采集源文件
* 说明:  采集的引脚为:PB9 可在DHT11.h文件中修改
* 说明:  采集的温湿度数据存储--DHT11Data[4]
* 说明:  None
* 说明:  None
* 支持:  QQ946029359 --群 607064330
* 淘宝:  https://shop411638453.taobao.com/
* 作者:  小五
**/


u8 DHT11Data[4]={0};//温湿度数据(湿度高位,湿度低位,温度高位,温度低位)
u8 GatherRrrorCnt = 0;//采集出错的次数
u8 LastT=0,LastR=0;//记录上一次的温湿度

/**
* @brief  DHT11开始信号
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
void DHT11_Rst(void)	   
{                 
	PIN_FUNC_SELECT(OW_MASTER_MUX, OW_MASTER_FUNC);//普通IO
	GPIO_OUTPUT_SET(OW_MASTER_GPIO, 0);
	os_delay_us(10000);
	os_delay_us(10000);
	GPIO_OUTPUT_SET(OW_MASTER_GPIO, 1);
	os_delay_us(30);
}



u8 DHT11_Check(void) 	   
{   
	u8 retry=0;

	GPIO_DIS_OUTPUT(OW_MASTER_GPIO);
	PIN_PULLUP_EN(OW_MASTER_MUX);//上拉输入


	while (DHT11_DQ_IN&&retry<100){
		retry++;
		os_delay_us(1);
	} 
	
	if(retry>=100){
		return 1;
	}
	
	retry=0;
	while (!DHT11_DQ_IN&&retry<100){
		retry++;
		os_delay_us(1);
	}
	
	if(retry>=100){
		return 1;	 
	}   
	return 0;
}



u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100)
	{
		retry++;
		os_delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100)
	{
		retry++;
		os_delay_us(1);
	}
	os_delay_us(35);
	if(DHT11_DQ_IN)return 1;
	else return 0;		   
}

u8 DHT11_Read_Byte(void)    
{
	u8 i,dat;
	dat=0;
	for (i=0;i<8;i++) {
	dat<<=1; 
	dat|=DHT11_Read_Bit();
	}						    
	return dat;
}

void DHT11_Read_Data(void)    
{        
 	u8 buf[5];
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++){
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4]){
			DHT11Data[0]=buf[0];
			DHT11Data[1]=buf[1];
			DHT11Data[2]=buf[2];
			DHT11Data[3]=buf[3];
			LastT = DHT11Data[2];
			LastR = DHT11Data[0];
		}
		else{
			if(abs(LastT-DHT11Data[2])<3 && abs(LastR-DHT11Data[0])<3)//如果误差不是很大也认为是正确的数据
			{
				DHT11Data[0]=buf[0];
				DHT11Data[1]=buf[1];
				DHT11Data[2]=buf[2];
				DHT11Data[3]=buf[3];
				GatherRrrorCnt = 0;
			}
			else
			{
				GatherRrrorCnt++;
			}
		}
		if(GatherRrrorCnt>5)//超过5次采集错误
		{
			GatherRrrorCnt = 0;
			DHT11Data[0]=0xff;
			DHT11Data[1]=0xff;
			DHT11Data[2]=0xff;
			DHT11Data[3]=0xff;
		}
	}
}
