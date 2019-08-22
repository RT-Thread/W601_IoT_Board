/***************************************************************************** 
* 
* File Name : load.c 
* 
* Description: load bin file
* 
* Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
* All rights reserved. 
* 
* Author : dave 
* 
* Date : 2014-7-11
*****************************************************************************/ 
#include "string.h"
#include "wm_include.h"
#include "load.h"


struct BinHead_ST binhead;

void load_bin_head(void)
{
	u8 tmp[16];
	int i,j;
	u32	binlength = 0;
	char *block ;
	
	memset(tmp,0,sizeof(tmp));
	tls_fls_read(TLS_FLASH_BOOT_FOOTER_ADDR, tmp, 16);
	binlength = *(u32 *)(tmp +4);
	printf("\nlen = %d\n",binlength);
	
	block = tls_mem_alloc(1024);
	memset(block, 0, 1024);
	tls_fls_read(TLS_FLASH_FIRMWARE1_ADDR + binlength - 1024, (u8 *)block, 1024);
	for(i = 1024;i > 0;i --)
	{
		if(0x11 == *(block + i) && 0x11 == *(block + i - 1) && 0x11 == *(block + i - 2))	//定位到head尾部
		{
			i -= 59;		//到head 信息的头部
			memset(&binhead, 0 ,sizeof(struct BinHead_ST));
			memcpy((u8 *)&binhead, block + i , 57);
			printf("\nbin num=%d\n",binhead.num);
			for(i = 0;i < binhead.num;i ++)
			{
				printf("\nbin[%d] offset=%x,len=%x\n",i, binhead.headinfo[i].offset,binhead.headinfo[i].len);
			}
			break;
		}
	}

	tls_mem_free(block);
}

int load_bin(void)
{
	u8 tmp[64];
	int i;
	
	memset(tmp,0,sizeof(tmp));
	tls_fls_read(TLS_FLASH_FIRMWARE1_ADDR + binhead.headinfo[0].offset - 10, (u8 *)tmp, 64);
	for(i = 0;i < 64;i ++)
	{
		printf("[%x]",tmp[i]);
		if(0 == i%32 && i > 0)
			printf("\n");
	}
	printf("\n");
	tls_fls_read(TLS_FLASH_FIRMWARE1_ADDR + binhead.headinfo[1].offset - 10, (u8 *)tmp, 64);
	for(i = 0;i < 64;i ++)
	{
		printf("[%x]",tmp[i]);
		if(0 == i%32 && i > 0)
			printf("\n");
	}
	printf("\n");
	tls_fls_read(TLS_FLASH_FIRMWARE1_ADDR + binhead.headinfo[2].offset - 10, (u8 *)tmp, 64);
	for(i = 0;i < 64;i ++)
	{
		printf("[%x]",tmp[i]);
		if(0 == i%32 && i > 0)
			printf("\n");
	}

	return WM_SUCCESS;
}



