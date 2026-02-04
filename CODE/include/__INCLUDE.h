#ifndef __INCLUDE_H__
#define __INCLUDE_H__

/*
 * __INCLUDE.h 统一依赖入口文件
 *
 * 说明：
 * 1) 本文件集中管理工程内常用的系统、RTOS、驱动、应用与组件依赖。
 * 2) 业务源文件只需包含本文件即可获得完整依赖。
 * 3) 新增模块或第三方库时，请在此处统一补充头文件。
 *
 * RTOS 依赖说明：
 * - 一般只需包含 c2000_freertos.h，内部会带入 FreeRTOS 内核与常用模块头文件。
 */

/* ----------------------------- 标准库头文件 ----------------------------- */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* --------------------------- C2000 设备/驱动库 -------------------------- */
#include "driverlib.h"
#include "device.h"
#include "board.h"

/* --------------------------------- RTOS -------------------------------- */
#include "c2000_freertos.h"

/* ----------------------------- 项目驱动层 ------------------------------ */
#include "drv_epwm.h"
#include "drv_eqep.h"
#include "drv_spi.h"
#include "drv_sci.h"

/* ------------------------------ 应用层头文件 ----------------------------- */
#include "app_log.h"
#include "app_proto.h"
#include "app_pintest.h"
#include "app_FOC.h"
#include "app_FOC_OpenLoop.h"
#include "app_FOC_CloseLoop.h"

/* ------------------------------ 组件/算法库 ------------------------------ */
#include "types.h"
#include "filter_fo.h"
#include "pid.h"
#include "math.h"
#include "drv8316s.h"

#endif /* __INCLUDE_H__ */
