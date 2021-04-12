/*
 * aly_hmac.h
 *
 *  Created on: 2020Äê10ÔÂ13ÈÕ
 *      Author: yang
 */

#ifndef APP_INCLUDE_DRIVER_ALY_HMAC_H_
#define APP_INCLUDE_DRIVER_ALY_HMAC_H_

#include "osapi.h"
#include "os_type.h"

#ifndef _ALY_HMAC_H_
#define _ALY_HMAC_H_

#ifndef _ALY_HMAC_C_
#define _ALY_HMAC_Cx_ extern
#else
#define _ALY_HMAC_Cx_
#endif

#define KEY_IOPAD_SIZE 64
#define SHA1_DIGEST_SIZE 20
#define MD5_DIGEST_SIZE 16


void aly_hmac_sha1(const char *msg, uint32_t msg_len, const char *key, uint32_t key_len,unsigned char *output);
void aly_hmac_md5(const char *msg, uint32_t msg_len, const char *key, uint32_t key_len,unsigned char *output);

#endif



#endif /* APP_INCLUDE_DRIVER_ALY_HMAC_H_ */
