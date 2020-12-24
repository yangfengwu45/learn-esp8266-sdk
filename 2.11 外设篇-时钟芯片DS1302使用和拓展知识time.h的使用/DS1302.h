/*
 * DS1302.h
 *
 *  Created on: 2020年12月23日
 *      Author: yang
 */

#ifndef APP_INCLUDE_DRIVER_DS1302_H_
#define APP_INCLUDE_DRIVER_DS1302_H_


#ifndef DS1302_C_
#define DS1302_Cx_ extern
#else
#define DS1302_Cx_
#endif

#include "c_types.h"

//移植其它单片机,请修改下面的程序
/*GPIO14 -- RST*/
#define DS1302_RST_MUX PERIPHS_IO_MUX_MTMS_U
#define DS1302_RST_GPIO 14
#define DS1302_RST_FUNC FUNC_GPIO14
/*GPIO12 -- DTA*/
#define DS1302_DAT_MUX PERIPHS_IO_MUX_MTDI_U
#define DS1302_DAT_GPIO 12
#define DS1302_DAT_FUNC FUNC_GPIO12
/*GPIO13 -- CLK*/
#define DS1302_CLK_MUX PERIPHS_IO_MUX_MTCK_U
#define DS1302_CLK_GPIO 13
#define DS1302_CLK_FUNC FUNC_GPIO13


//命令, 地址
#define ds1302_sec_add			0x80		//秒数据地址
#define ds1302_min_add			0x82		//分数据地址
#define ds1302_hr_add			0x84		//时数据地址
#define ds1302_date_add			0x86		//日数据地址
#define ds1302_month_add		0x88		//月数据地址
#define ds1302_day_add			0x8a		//星期数据地址
#define ds1302_year_add			0x8c		//年数据地址
#define ds1302_control_add		0x8e		//控制数据地址
#define ds1302_charger_add		0x90		//充电寄存区
#define ds1302_burstwr_add		0xbe		//突发模式写数据
#define ds1302_burstre_add		0xbf		//突发模式读数据
#define ds1302_ram_add			0xc0		//RAM地址
//配置
#define ds1302_clkoff		0x80		//暂停时钟
#define ds1302_lock			0x80		//打开写保护
#define ds1302_unlock		0x00		//关闭写保护
#define ds1302_lv6			0xa5		//0.7v压降,2K阻值,2.15ma
#define ds1302_lv5			0xa9		//1.4v压降,2K阻值,1.80ma
#define ds1302_lv4			0xa6		//0.7v压降,4K阻值,1.07ma
#define ds1302_lv3			0xaa		//1.7v压降,4K阻值,0.90ma
#define ds1302_lv2			0xa7		//0.7v压降,8K阻值,0.50ma

struct ds1302struct
{
  int	tm_sec;//秒
  int	tm_min;//分
  int	tm_hour;//时
  int	tm_mday;//日
  int	tm_mon;//月
  int	tm_year;//年
  int	tm_wday;//星期
};


//初始化引脚,移植其它单片机请替换内部函数
void ds1302_gpio_init(void);
//延时us,移植其它单片机请替换内部函数
void ds1302_delay_us(char us);
//设置时钟引脚输出高低电平,移植其它单片机请替换内部函数
void ds1302_clk_set(char HL);
//设置RST引脚输出高低电平,移植其它单片机请替换内部函数
void ds1302_rst_set(char HL);
//设置数据引脚输出高低电平,移植其它单片机请替换内部函数
void ds1302_dat_set(char HL);
//获取数据引脚的高低电平,移植其它单片机请替换内部函数
char ds1302_dat_get();


/**
* @brief  写一个字节数据到芯片
* @param  data:要写入的数据
* @param  None
* @param  None
* @retval None
* @example
**/
void ds1302_write_byte(char data);


/**
* @brief  读取一个字节数据
* @param  None
* @param  None
* @param  None
* @retval None
* @example
**/
char ds1302_read_byte();


/**
* @brief  写数据到芯片
* @param  addr:要写入的地址
* @param  data:要写入的数据
* @param  rc:1-操作内部RAM
* @retval None
* @example
**/
void ds1302_write_data(char addr,char data,char rc);


/**
* @brief  读取数据
* @param  addr:读取的地址
* @param  rc:1-操作内部RAM
* @param  None
* @retval None
* @example
**/
char ds1302_read_data(char addr,char rc);


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
void ds1302_set_time(struct ds1302struct *lcTime,char model);


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
void ds1302_read_time(struct ds1302struct *lcTime,char model);


#endif /* APP_INCLUDE_DRIVER_DS1302_H_ */
