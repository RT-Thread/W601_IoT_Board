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
#ifndef LOAD_H
#define LOAD_H

#include "wm_type_def.h"


 struct BinHeadInfo_ST{
	int offset;
	int len;
};

 struct BinHead_ST{
	struct BinHeadInfo_ST	headinfo[7];
	char				num;
};

void load_bin_head(void);
int load_bin(void);

#endif

