/*
 * aly_hmac.c
 *
 *  Created on: 2020Äê10ÔÂ13ÈÕ
 *      Author: yang
 */


#define _ALY_HMAC_C_
#include "driver/aly_hmac.h"
#include <stdio.h>
#include <string.h>
#include "osapi.h"
#include "driver/sha1.h"
#include "driver/md5.h"


void ICACHE_FLASH_ATTR
core_hex2str(uint8_t *input, uint32_t input_len, char *output, uint8_t lowercase){
    char *upper = "0123456789ABCDEF";
    char *lower = "0123456789abcdef";
    char *encode = upper;
    int i = 0, j = 0;

    if (lowercase) {
        encode = lower;
    }

    for (i = 0; i < input_len; i++) {
        output[j++] = encode[(input[i] >> 4) & 0xf];
        output[j++] = encode[(input[i]) & 0xf];
    }
}


void ICACHE_FLASH_ATTR
aly_hmac_sha1(const char *msg, uint32_t msg_len, const char *key, uint32_t key_len,unsigned char *output){
    mbedtls_sha1_context context;
		unsigned char sign[SHA1_DIGEST_SIZE+1] = {0};
    unsigned char k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    unsigned char k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    int32_t i;
		memset(sign, 0, sizeof(sign));
    if ((NULL == msg) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        return;
    }

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    mbedtls_sha1_init(&context);                                      /* init context for 1st pass */
    mbedtls_sha1_starts(&context);                                    /* setup context for 1st pass */
    mbedtls_sha1_update(&context, k_ipad, KEY_IOPAD_SIZE);     /* start with inner pad */
    mbedtls_sha1_update(&context, (unsigned char *)msg, msg_len);                      /* then text of datagram */
    mbedtls_sha1_finish(&context, sign);                            /* finish up 1st pass */

    /* perform outer SHA */
    mbedtls_sha1_init(&context);                              /* init context for 2nd pass */
    mbedtls_sha1_starts(&context);                            /* setup context for 2nd pass */
    mbedtls_sha1_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    mbedtls_sha1_update(&context, sign, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
    mbedtls_sha1_finish(&context, sign);                       /* finish up 2nd pass */

		core_hex2str(sign,SHA1_DIGEST_SIZE,(char *)output,0);
}


void ICACHE_FLASH_ATTR
aly_hmac_md5(const char *msg, uint32_t msg_len, const char *key, uint32_t key_len,unsigned char *output){
    mbedtls_md5_context context;

		unsigned char sign[MD5_DIGEST_SIZE+1] = {0};
    unsigned char k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    unsigned char k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    int32_t i;
		memset(sign, 0, sizeof(sign));
    if ((NULL == msg) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        return;
    }

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner MD5 */
    mbedtls_md5_init(&context);                                      /* init context for 1st pass */
    mbedtls_md5_starts(&context);                                    /* setup context for 1st pass */
    mbedtls_md5_update(&context, k_ipad, KEY_IOPAD_SIZE);     /* start with inner pad */
    mbedtls_md5_update(&context, (unsigned char *)msg, msg_len);                      /* then text of datagram */
    mbedtls_md5_finish(&context, sign);                            /* finish up 1st pass */

    /* perform outer MD5 */
    mbedtls_md5_init(&context);                              /* init context for 2nd pass */
    mbedtls_md5_starts(&context);                            /* setup context for 2nd pass */
    mbedtls_md5_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    mbedtls_md5_update(&context, sign, MD5_DIGEST_SIZE);     /* then results of 1st hash */
    mbedtls_md5_finish(&context, sign);                       /* finish up 2nd pass */

		core_hex2str(sign,MD5_DIGEST_SIZE,(char *)output,0);
}



