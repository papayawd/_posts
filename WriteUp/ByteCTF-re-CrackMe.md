---
title: ByteCTF re-CrackMe
date: 2020-10-29 17:20:26
tags: 十月姐姐温柔要求RE复现计划
---

比赛的时候已经知道怎么爆破了，结果用了python+常见字典。。。直接入坟

​	<!-- more -->

程序流程是吧passwordsha256加密之后，前半部分作为key，后半部分作为iv，进行AES-CBC加密。

由hint得知这个sha256是被修改过的，我们进入发现当password长度为32时候触发特殊的sha256

![](https://i.loli.net/2020/10/29/Y3Aezc17qxpKCBg.png)

可以在apk里面验证一下，输入32位的password，前四位固定，后边28为随机，sha256的结果都是一样的。

这里我们就可以只爆破4位去sha256加密即可。

sha256的final函数里面 源代码修改一下，使得datalen始终为32，并且正常padding

```c
//sha256.h

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32 // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE; // 8-bit byte
typedef unsigned int WORD;  // 32-bit word, change to "long" for 16-bit machines

typedef struct
{
    BYTE data[64];             // current 512-bit chunk of message data, just like a buffer
    WORD datalen;              // sign the data length of current chunk
    unsigned long long bitlen; // the bit length of the total message
    WORD state[8];             // store the middle state of hash abstract
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

/**************************** VARIABLES *****************************/
static const WORD k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

/*********************** FUNCTION DEFINITIONS ***********************/
void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
    WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    // initialization
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; ++i)
    {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
    WORD i;

    for (i = 0; i < len; ++i)
    {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64)
        {
            // 64 byte = 512 bit  means the buffer ctx->data has fully stored one chunk of message
            // so do the sha256 hash map for the current chunk
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
    WORD i;

    i = ctx->datalen;
  	ctx->datalen = 32 //这行是新加的

    // Pad whatever data is left in the buffer.
    if (i < 56)       //  这是修改的代码
 // if (ctx->datalen < 56)   这是原来的代码
    {
        ctx->data[i++] = 0x80; // pad 10000000 = 0x80
        while (i < 56)
            ctx->data[i++] = 0x00;
    }
    else
    {
        ctx->data[i++] = 0x80;
        while (i < 64)
            ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);

    // copying the final state to the output hash(use big endian).
    for (i = 0; i < 4; ++i)
    {
        hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
}

#endif // SHA256_H
```

爆破程序

这里学习wp的爆破范围为  0x30～0x7a   但是 0x00～0xff 的爆破时间也是可行的

```c
/*************************** HEADER FILES ***************************/

#include "sha256.h"
#include <openssl/aes.h>
#include <stdio.h>
#include <string.h>
unsigned char enc [33] =
        {
        0x2d, 0x18, 0x6a, 0x3e, 0x17, 0x2a, 0x14, 0x67, 0x37, 0x89, 0xf4, 0x99, 0xcd, 0x6c, 0xfb, 0xcd,
        0x29, 0xb6, 0xc7, 0x3f, 0x4b, 0x4a, 0x27, 0xc2, 0x34, 0x64, 0x77, 0x68, 0x25, 0xaf, 0x90, 0xb2
        };
unsigned char text[] = "2d186a3e172a14673789f499cd6cfbcd29b6c73f4b4a27c23464776825af90b2";
unsigned char encode_data[33];
unsigned char decode_data[33];
unsigned char hash_data[33];
unsigned char key[17];
unsigned char iv[17];
unsigned char buf[33];
unsigned char s[5] = "9hju";
AES_KEY aes_key;
SHA256_CTX ctx;

int GetValue(char ch)
{
	if('0' <= ch && ch <= '9')
		return ch - '0';
	return ch - 'a' + 10;
}
void HexToStr(unsigned char* hex,unsigned char* data)
{
	for(int i = 0; i < strlen(hex); i += 2)
		{
			data[i >> 1] = (GetValue(hex[i]) << 4 ) + GetValue(hex[i+1]);
		}
	data[strlen(hex) >> 1] = '\0';
}
void StrToHex(unsigned char* str)
{
	for(int i = 0;i < 32 ; i++)
		printf("%02X ",str[i]);
	printf("\n");
}
int main()
{
	//9hju5555555555555555555555555556
	/*
	sha256_init(&ctx);
	sha256_update(&ctx,s,4);
	sha256_final(&ctx, buf);
	StrToHex(buf);
	return 0;
	*/
	HexToStr(text,encode_data);
	for(int c1 = 0x30; s[0] = c1, c1 < 0x7a; c1++)
		{

			for(int c2 = 0x30; s[1] = c2, c2 < 0x7a; c2++)
				for(int c3 = 0x30; s[2] = c3, c3 < 0x7a; c3++)
					for(int c4 = 0x30; s[3] = c4, c4 < 0x7a; c4++)
						{
							sha256_init(&ctx);
							sha256_update(&ctx,s,4);
							sha256_final(&ctx, buf);
							memcpy(key, buf, 16);
							memcpy(iv, &buf[16], 16);
							AES_set_decrypt_key(key,128,&aes_key);
							AES_cbc_encrypt(encode_data,decode_data,(size_t)strlen(encode_data),&aes_key,iv,AES_DECRYPT);
							//printf("encode_data : %s\n",encode_data);
							//printf("decode_data : %s\n",decode_data);
							if(!memcmp("ByteCTF",decode_data,7))
								{
									printf("_____________________\n");
									printf("%s\n",decode_data);
									return 0;
								}
						}
			printf("0x%x\n",c1);
		}
		
	return 0;
}
```

