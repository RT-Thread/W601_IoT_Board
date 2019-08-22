#ifndef TC_IOT_HAL_OS_H
#define TC_IOT_HAL_OS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "qcloud_iot_import.h"



void *HAL_MutexCreate(void);
void HAL_MutexDestroy(void *mutex);
void HAL_MutexLock(void *mutex);
void HAL_MutexUnlock(void *mutex);
void *HAL_Malloc(uint32_t size);
void HAL_Free( void *ptr);
void HAL_Printf( const char *fmt, ...);
//int HAL_Snprintf( char *str, const int len, const char *fmt, ...);
void HAL_SleepMs( uint32_t ms);









#endif /* end of include guard */
