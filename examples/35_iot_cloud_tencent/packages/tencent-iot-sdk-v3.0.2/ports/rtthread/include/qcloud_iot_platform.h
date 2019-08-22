#ifndef TC_IOT_PLATFORM_H
#define TC_IOT_PLATFORM_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define tc_iot_hal_malloc malloc
//#define tc_iot_hal_free free
//#define tc_iot_hal_printf printf
//#define HAL_Printf printf
//#define HAL_Snprintf snprintf

/* Signed decimal notation. */ 
#ifndef SCNi8
#define SCNi8 "hhi"
#endif

#ifndef SCNu8
#define SCNu8 "hhu"  
#endif

/**
 * 定义特定平台下的一个定时器结构体,
 */
typedef int 			intptr_t;
typedef unsigned int	uintptr_t;



struct qTimer {
    uintptr_t end_time;
};

typedef struct qTimer	Timer;








#endif /* end of include guard */
