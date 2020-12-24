/*
 * DS1302.C
 *
 *  Created on: 2020年12月23日
 *      Author: yang
 *      使用:设置时间
		struct ds1302struct lcTime;
		lcTime.tm_year = 20;//年(芯片只能存储两位 00-99,具体看手册)
		lcTime.tm_mon = 12;//月
		lcTime.tm_mday = 23;//日
		lcTime.tm_hour = 22;//时
		lcTime.tm_min = 55;//分
		lcTime.tm_sec = 5;//秒
		ds1302_set_time(&lcTime,1);

		使用:获取时间
		struct ds1302struct lcTime;
		ds1302_read_time(&lcTime,1);//获取时钟的时间
		//打印 年 月 日 时 分 秒
		os_printf("\nyear=%d,mon=%d,mday=%d,hour=%d,min=%d,sec=%d\n",
				lcTime.tm_year,
				lcTime.tm_mon,
				lcTime.tm_mday,
				lcTime.tm_hour,
				lcTime.tm_min,
				lcTime.tm_sec
				);
 */
#define DS1302_C_
#include <time.h>
#include "driver/DS1302.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "gpio.h"

//初始化引脚,移植其它单片机请替换内部函数
void ICACHE_FLASH_ATTR
ds1302_gpio_init(void) {
	ETS_GPIO_INTR_DISABLE();

	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);

    PIN_FUNC_SELECT(DS1302_RST_MUX, DS1302_RST_FUNC);
	PIN_FUNC_SELECT(DS1302_DAT_MUX, DS1302_DAT_FUNC);
	PIN_FUNC_SELECT(DS1302_CLK_MUX, DS1302_CLK_FUNC);

	GPIO_OUTPUT_SET(DS1302_RST_GPIO, 0);
	GPIO_OUTPUT_SET(DS1302_DAT_GPIO, 0);
	GPIO_OUTPUT_SET(DS1302_CLK_GPIO, 0);

    ETS_GPIO_INTR_ENABLE() ;
}

//延时us,移植其它单片机请替换内部函数
void ICACHE_FLASH_ATTR
ds1302_delay_us(char us){
	os_delay_us(us);
}
//设置时钟引脚输出高低电平,移植其它单片机请替换内部函数
void ICACHE_FLASH_ATTR
ds1302_clk_set(char HL){
	if(HL==1)
		GPIO_OUTPUT_SET(DS1302_CLK_GPIO, 1);
	else
		GPIO_OUTPUT_SET(DS1302_CLK_GPIO, 0);
}
//设置RST引脚输出高低电平,移植其它单片机请替换内部函数
void ICACHE_FLASH_ATTR
ds1302_rst_set(char HL){
	if(HL==1)
		GPIO_OUTPUT_SET(DS1302_RST_GPIO, 1);
	else
		GPIO_OUTPUT_SET(DS1302_RST_GPIO, 0);
}
//设置数据引脚输出高低电平,移植其它单片机请替换内部函数
void ICACHE_FLASH_ATTR
ds1302_dat_set(char HL){
	if(HL==1)
		GPIO_OUTPUT_SET(DS1302_DAT_GPIO, 1);
	else
		GPIO_OUTPUT_SET(DS1302_DAT_GPIO, 0);
}

//获取数据引脚的高低电平,移植其它单片机请替换内部函数
char ICACHE_FLASH_ATTR
ds1302_dat_get(){
	return GPIO_INPUT_GET(DS1302_DAT_GPIO);
}


/**
* @brief  写一个字节数据到芯片
* @param  data:要写入的数据
* @param  None
* @param  None
* @retval None
* @example
**/
void ICACHE_FLASH_ATTR
ds1302_write_byte(char data){
	char i=0;

	for (i = 0; i < 8; i++){
		ds1302_clk_set(0);//先保持CLK为低电平
		ds1302_delay_us(1);

		//准备数据
		if (data & 0x01)
			ds1302_dat_set(1);
		else
			ds1302_dat_set(0);

		//产生上升沿
		ds1302_delay_us(1);
		ds1302_clk_set(1);
		ds1302_delay_us(1);

		data = data >> 1;
	}
}


/**
* @brief  读取一个字节数据
* @param  None
* @param  None
* @param  None
* @retval None
* @example
**/
char ICACHE_FLASH_ATTR
ds1302_read_byte(){
	char i=0, temp=0;

	//读取数据
	for (i = 0; i < 8;i++) {
		ds1302_clk_set(0);//产生了第一个下降沿
		ds1302_delay_us(1);

		temp = temp >> 1;
		if (ds1302_dat_get()){//获取数据
			temp |= 0x80;
		}

		ds1302_delay_us(1);
		ds1302_clk_set(1);
		ds1302_delay_us(1);
	}
	return temp;
}

/**
* @brief  写数据到芯片
* @param  addr:要写入的地址
* @param  data:要写入的数据
* @param  rc:1-操作内部RAM
* @retval None
* @example
**/
void ICACHE_FLASH_ATTR
ds1302_write_data(char addr,char data,char rc){
	ds1302_clk_set(0);//先保持CLK为低电平
	ds1302_delay_us(1);
	ds1302_rst_set(1);//启用传输
	ds1302_delay_us(1);

	addr = addr & 0xFE;//数据最低位,置0(写入数据)
	if(rc==1) addr = addr | 0x40; //操作RAM
	ds1302_write_byte(addr);
	ds1302_write_byte(data);

	ds1302_rst_set(0);//停止传输
	ds1302_delay_us(1);
}


/**
* @brief  读取数据
* @param  addr:读取的地址
* @param  rc:1-操作内部RAM
* @param  None
* @retval None
* @example
**/
char ICACHE_FLASH_ATTR
ds1302_read_data(char addr,char rc){
	char temp;
	ds1302_clk_set(0);//先保持CLK为低电平
	ds1302_delay_us(1);
	ds1302_rst_set(1);//启用传输
	ds1302_delay_us(1);

	addr = addr | 0x01;//数据最低位置1(读取数据)
	if(rc==1) addr = addr | 0x40; //操作RAM
	ds1302_write_byte(addr);

	temp = ds1302_read_byte();

	ds1302_rst_set(0);//停止传输
	ds1302_delay_us(1);
	return temp;
}

/**
* @brief  写数据到芯片(突发模式)
* @param  data:要写入的8字节数据
* @param  None
* @param  None
* @retval None
* @example
**/
void ICACHE_FLASH_ATTR
ds1302_write_data_burst(char *data){
	int i=0;
	ds1302_clk_set(0);//先保持CLK为低电平
	ds1302_delay_us(1);
	ds1302_rst_set(1);//启用传输
	ds1302_delay_us(1);

	ds1302_write_byte(ds1302_burstwr_add);

	for (i=0; i<8; i++){ //连续写入 8 字节数据
		ds1302_write_byte(data[i]);
	}

	ds1302_rst_set(0);//停止传输
	ds1302_delay_us(1);
}

/**
* @brief  读取数据(突发模式)
* @param  data:读取的地址
* @param  None
* @param  None
* @retval None
* @example
**/
void ICACHE_FLASH_ATTR
ds1302_read_data_burst(char *data){
	int i=0;
	ds1302_clk_set(0);//先保持CLK为低电平
	ds1302_delay_us(1);
	ds1302_rst_set(1);//启用传输
	ds1302_delay_us(1);

	ds1302_write_byte(ds1302_burstre_add);
	for (i=0; i<8; i++){ //连续写入 8 字节数据
		data[i] = ds1302_read_byte();
	}

	ds1302_rst_set(0);//停止传输
	ds1302_delay_us(1);
}

/**
* @brief  设置芯片的时间
* @param  tm
* @param  model:0-普通模式  1-突发模式
* @param  None
* @retval None
* @example
* struct ds1302struct lcTime;
* lcTime.tm_year = 2020;
* lcTime.tm_mon = 1;
* lcTime.tm_mday = 1;
* lcTime.tm_hour = 12;
* lcTime.tm_min = 10;
* lcTime.tm_sec = 30;
* ds1302_set_time(&lcTime,1);
**/
void ICACHE_FLASH_ATTR
ds1302_set_time(struct ds1302struct *lcTime,char model){
	char temp[8];//突发模式下缓存数据
//	char value,valueH,valueL;
//因为芯片高四位存储十位,低四位存储个位
//假设用户输入12  那么需要高位是 0001  低位是 0010   也就是0x12  然后存储到芯片中
//	valueH = (lcTime->tm_year/10)<<4;
//	valueL = lcTime->tm_year%10;
//	value = valueH | valueL;
	if(model){//突发模式
		temp[0] = (lcTime->tm_sec/10)<<4|lcTime->tm_sec%10;		//秒
		temp[1] = (lcTime->tm_min/10)<<4|lcTime->tm_min%10;		//分
		temp[2] = (lcTime->tm_hour/10)<<4|lcTime->tm_hour%10;	//时
		temp[3] = (lcTime->tm_mday/10)<<4|lcTime->tm_mday%10;	//日
		temp[4] = (lcTime->tm_mon/10)<<4|lcTime->tm_mon%10;		//月
		temp[5] = (lcTime->tm_wday/10)<<4|lcTime->tm_wday%10;	//星期
		temp[6] = (lcTime->tm_year/10)<<4|lcTime->tm_year%10;	//年
		//最后一位是写保护寄存器,全是0就可以,所以不需要设置
		ds1302_write_data_burst(temp);
	}
	else{
		ds1302_write_data(ds1302_control_add,ds1302_unlock,0);		//关闭写保护
		ds1302_write_data(ds1302_sec_add,ds1302_clkoff,0);			//暂停时钟
		//ds1302_write_data(ds1302_charger_add,ds1302_lv5);	    //涓流充电
		ds1302_write_data(ds1302_year_add,(lcTime->tm_year/10)<<4|lcTime->tm_year%10,0);//年
		ds1302_write_data(ds1302_month_add,(lcTime->tm_mon/10)<<4|lcTime->tm_mon%10,0);	//月
		ds1302_write_data(ds1302_date_add,(lcTime->tm_mday/10)<<4|lcTime->tm_mday%10,0);//日
		ds1302_write_data(ds1302_hr_add,(lcTime->tm_hour/10)<<4|lcTime->tm_hour%10,0);	//时
		ds1302_write_data(ds1302_min_add,(lcTime->tm_min/10)<<4|lcTime->tm_min%10,0);	//分
		ds1302_write_data(ds1302_sec_add,(lcTime->tm_sec/10)<<4|lcTime->tm_sec%10,0);	//秒
		ds1302_write_data(ds1302_day_add,(lcTime->tm_wday/10)<<4|lcTime->tm_wday%10,0);	//周
		ds1302_write_data(ds1302_control_add,ds1302_lock,0);		//打开写保护
	}
}


/**
* @brief  获取芯片的时间
* @param  tm
* @param  model:0-普通模式  1-突发模式
* @param  None
* @retval None
* @example
* struct ds1302struct lcTime;
* ds1302_read_time(&lcTime,1);
* int year = lcTime.tm_year;
* int mon = lcTime.tm_mon;
* int mday = lcTime.tm_mday;
* char hour = lcTime.tm_hour;
* char min = lcTime.tm_min;
* char sec = lcTime.tm_sec;
**/
void ICACHE_FLASH_ATTR
ds1302_read_time(struct ds1302struct *lcTime,char model){
	char temp[8];//突发模式下缓存数据
	char value,valueH,valueL;
	//提示:所有数据的高四位代表十位,低四位代表个位
	//有些数据的MSB最高位代表其它含义,请参见DS1302手册

	if(model){//突发模式
		ds1302_read_data_burst(temp);
		valueH = (temp[6]>>4)&0x0f;
		valueL = temp[6]&0x0f;
		lcTime->tm_year = valueH*10 + valueL;

		valueH = (temp[5]>>4)&0x0f;
		valueL = temp[5]&0x0f;
		lcTime->tm_wday = valueH*10 + valueL;

		valueH = (temp[4]>>4)&0x0f;
		valueL = temp[4]&0x0f;
		lcTime->tm_mon = valueH*10 + valueL;

		valueH = (temp[3]>>4)&0x0f;
		valueL = temp[3]&0x0f;
		lcTime->tm_mday = valueH*10 + valueL;

		valueH = (temp[2]>>4)&0x0f;
		valueL = temp[2]&0x0f;
		lcTime->tm_hour = valueH*10 + valueL;

		valueH = (temp[1]>>4)&0x0f;
		valueL = temp[1]&0x0f;
		lcTime->tm_min = valueH*10 + valueL;

		valueH = (temp[0]>>4)&0x0f;
		valueL = temp[0]&0x0f;
		lcTime->tm_sec = valueH*10 + valueL;
	}else{
		value = ds1302_read_data(ds1302_year_add,0);	 //年
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_year = valueH*10 + valueL;

		value = ds1302_read_data(ds1302_month_add,0);	 //月
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_mon = valueH*10 + valueL;

		value = ds1302_read_data(ds1302_date_add,0);	 //日
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_mday = valueH*10 + valueL;

		value = ds1302_read_data(ds1302_hr_add,0);		 //时
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_hour = valueH*10 + valueL;

		value = ds1302_read_data(ds1302_min_add,0);		 //分
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_min = valueH*10 + valueL;

		value = (ds1302_read_data(ds1302_sec_add,0))&0x7f;//秒，屏蔽秒的第7位，避免超出59
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_sec = valueH*10 + valueL;

		value = ds1302_read_data(ds1302_day_add,0);		 //周
		valueH = (value>>4)&0x0f;
		valueL = value&0x0f;
		lcTime->tm_wday = valueH*10 + valueL;
	}
}


