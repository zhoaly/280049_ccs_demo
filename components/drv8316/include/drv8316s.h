//! \file   libraries/drvic/drv8316/include/drv8316s.h
//! \brief  包含与 DRV8316 对象相关的各种函数的公共接口
//!         针对 DRV8316 对象
//!

#ifndef DRV8316S_H
#define DRV8316S_H


//*****************************************************************************
//
// 若使用 C++ 编译器构建，请确保此头文件中的所有定义均采用 C 语言链接。
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \defgroup DRVIC DRV8316
//! @{
//
//*****************************************************************************

// 引用的头文件
#include <math.h>

// 驱动
#include "spi.h"
#include "gpio.h"

// **************************************************************************
// 模块

// **************************************************************************
// 解决方案

// **************************************************************************
// 宏定义

//! \brief 定义地址掩码
//!
#define DRV8316_ADDR_MASK                   (0x7E00)

//! \brief 定义数据掩码
//!
#define DRV8316_DATA_MASK                   (0x00FF)

//! \brief 定义读写掩码
//!
#define DRV8316_RW_MASK                     (0x8000)

//
// 状态寄存器 00
//
//! \brief 定义读写掩码
//!
#define DRV8316_FAULT_TYPE_MASK             (0x00FF)

//! \brief 定义寄存器地址
//!
#define statReg00_addr      0x00
#define statReg01_addr      0x01
#define statReg02_addr      0x02

#define ctrlReg01_addr      0x03
#define ctrlReg02_addr      0x04
#define ctrlReg03_addr      0x05
#define ctrlReg04_addr      0x06
#define ctrlReg05_addr      0x07
#define ctrlReg06_addr      0x08
#define ctrlReg10_addr      0x0C

//
// 状态寄存器 00
//
#define DRV8316_STAT00_FAULT_BITS           (1 << 0)
#define DRV8316_STAT00_OT_BITS              (1 << 1)
#define DRV8316_STAT00_OVP_BITS             (1 << 2)
#define DRV8316_STAT00_NPOR_BITS            (1 << 3)
#define DRV8316_STAT00_OCP_BITS             (1 << 4)
#define DRV8316_STAT00_SPI_FLT_BITS         (1 << 5)
#define DRV8316_STAT00_BK_FLT_BITS          (1 << 6)
#define DRV8316_STAT00_RESERVED_BITS        (1 << 7)

//
// 状态寄存器 01
//
#define DRV8316_STAT01_OCP_LA_BITS          (1 << 0)
#define DRV8316_STAT01_OCP_HA_BITS          (1 << 1)
#define DRV8316_STAT01_OCP_LB_BITS          (1 << 2)
#define DRV8316_STAT01_OCP_HB_BITS          (1 << 3)
#define DRV8316_STAT01_OCP_LC_BITS          (1 << 4)
#define DRV8316_STAT01_OCP_HC_BITS          (1 << 5)
#define DRV8316_STAT01_OTS_BITS             (1 << 6)
#define DRV8316_STAT01_OTW_BITS             (1 << 7)

//
// 状态寄存器 02
//
#define DRV8316_STAT02_SPI_ADDR_FLT_BITS    (1 << 0)
#define DRV8316_STAT02_SPI_SCLK_FLT_BITS    (1 << 1)
#define DRV8316_STAT02_SPI_PARITY_BITS      (1 << 2)
#define DRV8316_STAT02_VCP_UV_BITS          (1 << 3)
#define DRV8316_STAT02_BUCK_UV_BITS         (1 << 4)
#define DRV8316_STAT02_BUCK_OCP_BITS        (1 << 5)
#define DRV8316_STAT02_OTP_ERR_BITS         (1 << 6)
#define DRV8316_STAT02_RESERVED_BITS        (1 << 7)

//
// 控制寄存器 03
//
#define DRV8316_CTRL03_REG_LOCK_BITS        (7 << 0)
#define DRV8316_CTRL03_RESERVED1_BITS       (1 << 3)
#define DRV8316_CTRL03_RESERVED2_BITS       (1 << 4)
#define DRV8316_CTRL03_RESERVED3_BITS       (1 << 5)
#define DRV8316_CTRL03_RESERVED4_BITS       (1 << 6)
#define DRV8316_CTRL03_RESERVED5_BITS       (1 << 7)

//
// 控制寄存器 04
//
#define DRV8316_CTRL04_CLR_FLT_BITS         (1 << 0)
#define DRV8316_CTRL04_PWM_MODE_BITS        (3 << 1)
#define DRV8316_CTRL04_SLEW_BITS            (3 << 3)
#define DRV8316_CTRL04_RESERVED1_BITS       (1 << 5)
#define DRV8316_CTRL04_RESERVED2_BITS       (1 << 6)
#define DRV8316_CTRL04_RESERVED3_BITS       (1 << 7)

//
// 控制寄存器 05
//
#define DRV8316_CTRL05_OTW_REP_BITS         (1 << 0)
#define DRV8316_CTRL05_SPI_FLT_REP_BITS     (1 << 1)
#define DRV8316_CTRL05_OVP_EN_BITS          (1 << 2)
#define DRV8316_CTRL05_OVP_SEL_BITS         (1 << 3)
#define DRV8316_CTRL05_RESERVED1_BITS       (1 << 4)
#define DRV8316_CTRL05_RESERVED2_BITS       (1 << 5)
#define DRV8316_CTRL05_RESERVED3_BITS       (1 << 6)
#define DRV8316_CTRL05_RESERVED4_BITS       (1 << 7)

//
// 控制寄存器 06
//
#define DRV8316_CTRL06_OCP_MODE_BITS        (3 << 0)
#define DRV8316_CTRL06_OCP_LVL_BITS         (1 << 2)
#define DRV8316_CTRL06_OCP_RETRY_BITS       (1 << 3)
#define DRV8316_CTRL06_OCP_DEG_BITS         (3 << 4)
#define DRV8316_CTRL06_OCP_CBC_BITS         (1 << 6)
#define DRV8316_CTRL06_RESERVED_BITS        (1 << 7)

//
// 控制寄存器 07
//
#define DRV8316_CTRL07_CSA_GAIN_BITS        (3 << 0)
#define DRV8316_CTRL07_EN_ASR_BITS          (1 << 2)
#define DRV8316_CTRL07_EN_AAR_BITS          (1 << 3)
#define DRV8316_CTRL07_AD_COMP_TH_HS_BITS   (1 << 4)
#define DRV8316_CTRL07_AD_COMP_TH_LS_BITS   (1 << 5)
#define DRV8316_CTRL07_ILIM_RECIR_BITS      (1 << 6)
#define DRV8316_CTRL07_BEMF_TH_BITS         (1 << 7)

//
// 控制寄存器 08
//
#define DRV8316_CTRL08_BUCK_DIS_BITS        (1 << 0)
#define DRV8316_CTRL08_BUCK_SEL_BITS        (3 << 1)
#define DRV8316_CTRL08_BUCK_CL_BITS         (1 << 3)
#define DRV8316_CTRL08_BUCK_PS_DIS_BITS     (1 << 4)
#define DRV8316_CTRL08_BUCK_SR_BITS         (1 << 5)
#define DRV8316_CTRL08_RESERVED1_BITS       (1 << 6)
#define DRV8316_CTRL08_RESERVED2_BITS       (1 << 7)

// **************************************************************************
// 类型定义

//------------------------------------------------------------------------------
//! \brief 枚举读写模式
//!
typedef enum
{
    DRV8316_CTRLMODE_WRITE    = (0 << 15),  //!< 写模式
    DRV8316_CTRLMODE_READ     = (1 << 15)   //!< 读模式
} DRV8316_CtrlMode_e;

//! \brief 枚举寄存器地址
//!
typedef enum
{
    DRV8316_ADDRESS_STATUS_0   = (0x0 << 9),  //!< 状态寄存器 0
    DRV8316_ADDRESS_STATUS_1   = (0x1 << 9),  //!< 状态寄存器 1
    DRV8316_ADDRESS_STATUS_2   = (0x2 << 9),  //!< 状态寄存器 2
    DRV8316_ADDRESS_CONTROL_1  = (0x3 << 9),  //!< 控制寄存器 1
    DRV8316_ADDRESS_CONTROL_2  = (0x4 << 9),  //!< 控制寄存器 2
    DRV8316_ADDRESS_CONTROL_3  = (0x5 << 9),  //!< 控制寄存器 3
    DRV8316_ADDRESS_CONTROL_4  = (0x6 << 9),  //!< 控制寄存器 4
    DRV8316_ADDRESS_CONTROL_5  = (0x7 << 9),  //!< 控制寄存器 5
    DRV8316_ADDRESS_CONTROL_6  = (0x8 << 9),  //!< 控制寄存器 6
    DRV8316_ADDRESS_CONTROL_10 = (0xC << 9)   //!< 控制寄存器 10
} DRV8316_Address_e;

//! \brief 枚举状态寄存器 0 的故障位
//!
typedef enum
{
    DRV8316_FAULT       = (1 << 0),    //!< 器件故障
    DRV8316_OT          = (1 << 1),    //!< 过温故障
    DRV8316_OVP         = (1 << 2),    //!< 电源过压保护
    DRV8316_NPOR        = (1 << 3),    //!< 上电复位
    DRV8316_OCP         = (1 << 4),    //!< 过流保护
    DRV8316_SPI_FLT     = (1 << 5),    //!< SPI 故障
    DRV8316_BK_FLT      = (1 << 6)     //!< Buck 故障
} DRV8316_STATUS00_e;

//! \brief 枚举状态寄存器 1 的故障位
//!
typedef enum
{
    DRV8316_OCP_LA      = (1 << 0),    //!< OUTA 低边开关过流状态
    DRV8316_OCP_HA      = (1 << 1),    //!< OUTA 高边开关过流状态
    DRV8316_OCP_LB      = (1 << 2),    //!< OUTB 低边开关过流状态
    DRV8316_OCP_HB      = (1 << 3),    //!< OUTB 高边开关过流状态
    DRV8316_OCL_LC      = (1 << 4),    //!< OUTC 低边开关过流状态
    DRV8316_OCP_HC      = (1 << 5),    //!< OUTC 高边开关过流状态
    DRV8316_OTS         = (1 << 6),    //!< 过温关断状态
    DRV8316_OTW         = (1 << 7)     //!< 过温警告状态
} DRV8316_STATUS01_e;

//! \brief 枚举状态寄存器 1 的故障位
//!
typedef enum
{
    DRV8316_SPI_ADDR_FLT  = (1 << 0),    //!< SPI 地址错误
    DRV8316_SPI_SCLK_FLT  = (1 << 1),    //!< SPI 时钟帧错误
    DRV8316_SPI_PARITY    = (1 << 2),    //!< SPI 奇偶校验错误
    DRV8316_VCP_UV        = (1 << 3),    //!< 充电泵欠压状态
    DRV8316_BUCK_UV       = (1 << 4),    //!< Buck 稳压器欠压状态
    DRV8316_BUCK_OCP      = (1 << 5),    //!< Buck 稳压器过流状态
    DRV8316_OTP_ERR       = (1 << 6),    //!< 一次性可编程错误
    DRV8316_RESERVED      = (1 << 7)     //!< 保留
} DRV8316_STATUS02_e;

//! \brief 枚举高边栅极驱动峰值源电流；
//! 栅极电流与数据手册不一致
//!
typedef enum
{
    DRV8316_REG_LOCK_0 = 0,  //!< 除解锁/锁定外无效果
    DRV8316_REG_LOCK_1 = 1,  //!< 除解锁/锁定外无效果
    DRV8316_REG_LOCK_2 = 2,  //!< 除解锁/锁定外无效果
    DRV8316_REG_LOCK_3 = 3,  //!< 写入 011b 以解锁全部寄存器
    DRV8316_REG_LOCK_4 = 4,  //!< 除解锁/锁定外无效果
    DRV8316_REG_LOCK_5 = 5,  //!< 除解锁/锁定外无效果
    DRV8316_REG_LOCK_6 = 6,  //!< 写入 110b 以忽略后续寄存器访问并锁定设置
    DRV8316_REG_LOCK_7 = 7   //!< 除解锁/锁定外无效果
} DRV8316_CTRL01_RegLock_e;

//! \brief 枚举驱动器 PWM 模式
//!
typedef enum
{
    DRV8316_PWMMODE_6_N  = 0,     //!< PWM_MODE = 6 路输入
    DRV8316_PWMMODE_6_LC = 1,     //!< PWM_MODE = 6 路输入并启用限流
    DRV8316_PWMMODE_3_N  = 2,     //!< PWM_MODE = 3 路输入
    DRV8316_PWMMODE_3_LC = 3,     //!< PWM_MODE = 3 路输入并启用限流
} DRV8316_CTRL02_PWMMode_e;

//! \brief 枚举转换斜率
//!
typedef enum
{
    DRV8316_SLEW_25V  = 0,      //!< 上升沿速率为 25 V/μs
    DRV8316_SLEW_50V  = 1,      //!< 上升沿速率为 50 V/μs
    DRV8316_SLEW_150V = 2,      //!< 上升沿速率为 150 V/μs
    DRV8316_SLEW_200V = 3       //!< 上升沿速率为 200 V/μs
} DRV8316_CTRL02_SlewRate_e;

//! \brief 枚举 SDO 模式配置
//!
typedef enum
{
    DRV8316_SDOMODE_OPEN_DRAIN  = 0,     //!< SDO 引脚为开漏模式
    DRV8316_SDOMODE_PUSH_PULL   = 1      //!< SDO 引脚为推挽模式
} DRV8316_CTRL02_SDOMode_e;


//! \brief 枚举过压阈值设置
//!
typedef enum
{
    DRV8316_OVPSEL_32V  = 0,        //!< VM 过压阈值为 32 V
    DRV8316_OVPSEL_20V   = 1        //!< VM 过压阈值为 20 V
} DRV8316_CTRL03_OVPSEL_e;


//! \brief 枚举 100% 占空比时的 PWM 频率
//!
typedef enum
{
    DRV8316_PWM_100_DUTY_SEL_20KHz  = 0,     //!< 20 kHz
    DRV8316_PWM_100_DUTY_SEL_40KHz   = 1      //!< 40 kHz
} DRV8316_CTRL03_DUTYSEL_e;


//! \brief 枚举过流保护故障处理选项
//!
typedef enum
{
    DRV8316_OCP_MODE_LATCH   = 0,   //!< 过流将导致锁存故障
    DRV8316_OCP_MODE_RETRY   = 1,   //!< 过流将自动重试
    DRV8316_OCP_MODE_REPORT  = 2,   //!< 仅报告过流，不执行动作
    DRV8316_OCP_MODE_NO      = 3    //!< 不报告过流且不执行动作
} DRV8316_CTRL04_OCPMODE_e;

//! \brief 枚举过流阈值设置
//!
typedef enum
{
    DRV8316_OCP_LVL_16A  = 0,     //!< 16 A
    DRV8316_OCP_LVL_24A  = 1      //!< 24 A
} DRV8316_CTRL04_OCPLVL_e;

//! \brief 枚举过流保护重试时间
//!
typedef enum
{
    DRV8316_OCP_RETRY_5ms   = 0,     //!< OCP 重试时间为 5 ms
    DRV8316_OCP_RETRY_500ms = 1      //!< OCP 重试时间为 500 ms
} DRV8316_CTRL04_OCPRETRY_e;

//! \brief 枚举过流保护去抖时间
//!
typedef enum
{
    DRV8316_OCP_DEG_0p2us  = 0,   //!< OCP 去抖时间为 0.2 μs
    DRV8316_OCP_DEG_0p6us  = 1,   //!< OCP 去抖时间为 0.6 μs
    DRV8316_OCP_DEG_1p1us  = 2,   //!< OCP 去抖时间为 1.1 μs
    DRV8316_OCP_DEG_1p6us  = 3    //!< OCP 去抖时间为 1.6 μs
} DRV8316_CTRL04_OCPDEGE_e;

//! \brief 枚举电流采样放大器增益设置
//!
typedef enum
{
    DRV8316_CSA_GAIN_0p15VpA   = 0,   //!< CSA 增益为 0.15 V/A
    DRV8316_CSA_GAIN_0p30VpA   = 1,   //!< CSA 增益为 0.30 V/A
    DRV8316_CSA_GAIN_0p60VpA   = 2,   //!< CSA 增益为 0.60 V/A
    DRV8316_CSA_GAIN_1p20VpA   = 3,   //!< CSA 增益为 1.20 V/A
} DRV8316_CTRL05_CSAGain_e;

//! \brief 枚举电流限制回流配置
//!
typedef enum
{
    DRV8316_ILIM_RECIR_Brake   = 0,     //!< 电流通过 FET 回流（制动模式）
    DRV8316_ILIM_RECIR_Coast   = 1      //!< 电流通过二极管回流（滑行模式）
} DRV8316_CTRL05_ILIMRECIR_e;


//! \brief 枚举 Buck 电压选择
//!
typedef enum
{
    DRV8316_BUCK_SEL_3p3V  = 0,   //!< Buck 输出电压为 3.3 V
    DRV8316_BUCK_SEL_5p0V  = 1,   //!< Buck 输出电压为 5.0 V
    DRV8316_BUCK_SEL_4p0V  = 2,   //!< Buck 输出电压为 4.0 V
    DRV8316_BUCK_SEL_5p7V  = 3,   //!< Buck 输出电压为 5.7 V
} DRV8316_CTRL06_BUCKSEL_e;

//! \brief 枚举电流限制回流配置
//!
typedef enum
{
    DRV8316_BUCK_CL_600mA   = 0,     //!< Buck 稳压器电流限值为 600 mA
    DRV8316_BUCK_CL_150mA   = 1      //!< Buck 稳压器电流限值为 150 mA
} DRV8316_CTRL06_BUCKCL_e;


//! \brief 枚举驱动延时补偿目标
//!
typedef enum
{
    DRV8316_DLY_TARGET_0p0us  = 0x0,   //!< 延时为 0 μs
    DRV8316_DLY_TARGET_0p4us  = 0x1,   //!< 延时为 0.4 μs
    DRV8316_DLY_TARGET_0p6us  = 0x2,   //!< 延时为 0.6 μs
    DRV8316_DLY_TARGET_0p8us  = 0x3,   //!< 延时为 0.8 μs
    DRV8316_DLY_TARGET_1p0us  = 0x4,   //!< 延时为 1.0 μs
    DRV8316_DLY_TARGET_1p2us  = 0x5,   //!< 延时为 1.2 μs
    DRV8316_DLY_TARGET_1p4us  = 0x6,   //!< 延时为 1.4 μs
    DRV8316_DLY_TARGET_1p6us  = 0x7,   //!< 延时为 1.6 μs
    DRV8316_DLY_TARGET_1p8us  = 0x8,   //!< 延时为 1.8 μs
    DRV8316_DLY_TARGET_2p0us  = 0x9,   //!< 延时为 2.0 μs
    DRV8316_DLY_TARGET_2p2us  = 0xA,   //!< 延时为 2.2 μs
    DRV8316_DLY_TARGET_2p4us  = 0xB,   //!< 延时为 2.4 μs
    DRV8316_DLY_TARGET_2p6us  = 0xC,   //!< 延时为 2.6 μs
    DRV8316_DLY_TARGET_2p8us  = 0xD,   //!< 延时为 2.8 μs
    DRV8316_DLY_TARGET_3p0us  = 0xE,   //!< 延时为 3.0 μs
    DRV8316_DLY_TARGET_3p2us  = 0xF    //!< 延时为 3.2 μs
} DRV8316_CTRL10_DLYTARGET_e;

//------------------------------------------------------------------------------
//! \brief DRV8316 STATUS00 寄存器对象
//!
struct DRV8316_STAT00_BITS {                    // 位域说明
    bool                FAULT:1;            // 位 0
    bool                OT:1;               // 位 1
    bool                OVP:1;              // 位 2
    bool                NPOR:1;             // 位 3
    bool                OCP:1;              // 位 4
    bool                SPI_FLT:1;          // 位 5
    bool                BK_FLT:1;           // 位 6
    bool                rsvd1:1;            // 位 7
};

union DRV8316_STAT00_REG
{
    uint16_t all;
    struct   DRV8316_STAT00_BITS bit;
};

//! \brief DRV8316 STATUS01 寄存器对象
//!
struct DRV8316_STAT01_BITS {                    // 位域说明
    bool                OCP_LA:1;           // 位 0
    bool                OCP_HA:1;           // 位 1
    bool                OCP_LB:1;           // 位 2
    bool                OCP_HB:1;           // 位 3
    bool                OCL_LC:1;           // 位 4
    bool                OCP_HC:1;           // 位 5
    bool                OTS:1;              // 位 6
    bool                OTW:1;              // 位 7
};

union DRV8316_STAT01_REG
{
    uint16_t all;
    struct   DRV8316_STAT01_BITS bit;
};

//! \brief DRV8316 STATUS02 寄存器对象
//!
struct DRV8316_STAT02_BITS {                    // 位域说明
    bool                SPI_ADDR_FLT:1;     // 位 0
    bool                SPI_SCLK_FLT:1;     // 位 1
    bool                SPI_PARITY:1;       // 位 2
    bool                VCP_UV:1;           // 位 3
    bool                BUCK_UV:1;          // 位 4
    bool                BUCK_OCP:1;         // 位 5
    bool                OTP_ERR:1;          // 位 6
    uint16_t            rsvd2:5;            // 位 7 保留
};

union DRV8316_STAT02_REG
{
    uint16_t all;
    struct   DRV8316_STAT02_BITS bit;
};

//! \brief DRV8316 CTRL01 寄存器对象
//!
struct DRV8316_CTRL01_BITS {                    // 位域说明
    DRV8316_CTRL01_RegLock_e    REG_LOCK:3;       // 位 2:0
    uint16_t                    rsvd1:5;          // 位 7:3 保留
};

union DRV8316_CTRL01_REG
{
    uint16_t all;
    struct   DRV8316_CTRL01_BITS bit;
};

//! \brief DRV8316 CTRL03 寄存器对象
//!
struct DRV8316_CTRL02_BITS {                    // 位域说明
    bool                      CLR_FLT:1;        // 位 0
    DRV8316_CTRL02_PWMMode_e  PWM_MODE:2;       // 位 2-1
    DRV8316_CTRL02_SlewRate_e SLEW:2;           // 位 4-3
    DRV8316_CTRL02_SDOMode_e  SDO_MODE:1;       // 位 5
    uint16_t                  rsvd1:2;          // 位 7:6 保留
};

union DRV8316_CTRL02_REG
{
    uint16_t all;
    struct   DRV8316_CTRL02_BITS bit;
};

//! \brief DRV8316 CTRL03 寄存器对象
//!
struct DRV8316_CTRL03_BITS {                    // 位域说明
    bool                      OTW_REP:1;            // 位 0
    bool                      SPI_FLT_REP:1;        // 位 1
    bool                      OVP_EN:1;             // 位 2
    DRV8316_CTRL03_OVPSEL_e   OVP_SEL:1;            // 位 3
    DRV8316_CTRL03_DUTYSEL_e  PWM_100_DUTY_SEL:1;   // 位 4
    uint16_t                  rsvd1:3;              // 位 7:5 保留
};

union DRV8316_CTRL03_REG
{
    uint16_t all;
    struct   DRV8316_CTRL03_BITS bit;
};

//! \brief DRV8316 CTRL04 寄存器对象
//!
struct DRV8316_CTRL04_BITS {                    // 位域说明
    DRV8316_CTRL04_OCPMODE_e   OCP_MODE:2;      // 位 1-0
    DRV8316_CTRL04_OCPLVL_e    OCP_LVL:1;       // 位 2
    DRV8316_CTRL04_OCPRETRY_e  OCP_RETRY:1;     // 位 3
    DRV8316_CTRL04_OCPDEGE_e   OCP_DEG:2;       // 位 5-4
    bool                       OCP_CBC:1;       // 位 6
    bool                       DRV_OFF:1;       // 位 7
};

union DRV8316_CTRL04_REG
{
    uint16_t all;
    struct   DRV8316_CTRL04_BITS bit;
};

//! \brief DRV8316 CTRL05 寄存器对象
//!
struct DRV8316_CTRL05_BITS {                    // 位域说明
    DRV8316_CTRL05_CSAGain_e   CSA_GAIN:2;      // 位 1-0
    bool                       EN_ASR:1;        // 位 2
    bool                       EN_AAR:1;        // 位 3
    uint16_t                   rsvd1:1;         // 位 4 保留
    uint16_t                   rsvd2:1;         // 位 5 保留
    DRV8316_CTRL05_ILIMRECIR_e ILIM_RECIR:1;    // 位 6
    uint16_t                   rsvd3:1;         // 位 7 保留
};

union DRV8316_CTRL05_REG
{
    uint16_t all;
    struct   DRV8316_CTRL05_BITS bit;
};

//! \brief DRV8316 CTRL06 寄存器对象
//!
struct DRV8316_CTRL06_BITS {                    // 位域说明
    bool                        BUCK_DIS:1;     // 位 0
    DRV8316_CTRL06_BUCKSEL_e    BUCK_SEL:2;     // 位 2-1
    DRV8316_CTRL06_BUCKCL_e     BUCK_CL:1;      // 位 3
    bool                        BUCK_PS_DIS:1;  // 位 4
    uint16_t                    rsvd1:3;        // 位 7:5 保留
};

union DRV8316_CTRL06_REG
{
    uint16_t all;
    struct   DRV8316_CTRL06_BITS bit;
};

//! \brief DRV8316 CTRL10 寄存器对象
//!
struct DRV8316_CTRL10_BITS {                    // 位域说明
    DRV8316_CTRL10_DLYTARGET_e  DLY_TARGET:4;   // 位 3-0
    bool                        DLYCMP_EN:1;    // 位 4
    uint16_t                    rsvd1:3;        // 位 7:5 保留
};

union DRV8316_CTRL10_REG
{
    uint16_t all;
    struct   DRV8316_CTRL10_BITS bit;
};

//! \brief DRV8316 寄存器与命令对象
//!
typedef struct _DRV8316_VARS_t_
{
    union DRV8316_STAT00_REG    statReg00;
    union DRV8316_STAT01_REG    statReg01;
    union DRV8316_STAT02_REG    statReg02;

    union DRV8316_CTRL01_REG    ctrlReg01;
    union DRV8316_CTRL02_REG    ctrlReg02;
    union DRV8316_CTRL03_REG    ctrlReg03;
    union DRV8316_CTRL04_REG    ctrlReg04;
    union DRV8316_CTRL05_REG    ctrlReg05;
    union DRV8316_CTRL06_REG    ctrlReg06;
    union DRV8316_CTRL10_REG    ctrlReg10;

    bool                writeCmd;
    bool                readCmd;
    bool                manWriteCmd;
    bool                manReadCmd;
    uint16_t            manWriteAddr;
    uint16_t            manReadAddr;
    uint16_t            manWriteData;
    uint16_t            manReadData;
}DRV8316_VARS_t;

//! \brief 定义 DRV8316_VARS_t 句柄
//!
typedef struct _DRV8316_VARS_t_ *DRV8316VARS_Handle;

//! \brief 定义 DRV8316 对象
//!
typedef struct _DRV8316_Obj_
{
    uint32_t  spiHandle;     //!< 串行外设接口句柄
    uint32_t  gpioNumber_CS; //!< 连接到 DRV8316 CS 引脚的 GPIO
    uint32_t  gpioNumber_EN; //!< 连接到 DRV8316 使能引脚的 GPIO
    bool      rxTimeOut;     //!< RX FIFO 的超时标志
    bool      enableTimeOut; //!< DRV8316 使能的超时标志
} DRV8316_Obj;

//! \brief 定义 DRV8316 句柄
//!
typedef struct _DRV8316_Obj_ *DRV8316_Handle;

//! \brief 定义 DRV8316 的字类型
//!
typedef  uint16_t    DRV_Word_t;

// **************************************************************************
// 全局变量

// **************************************************************************
// 函数原型

//! \brief     初始化 DRV8316 对象
//! \param[in] pMemory   指向 DRV8316 对象内存的指针
//! \param[in] numBytes  为 DRV8316 对象分配的字节数
//!                      对象，单位：字节
//! \return    DRV8316 对象句柄
extern DRV8316_Handle DRV8316_init(void *pMemory);

//! \brief     构建控制字
//! \param[in] ctrlMode  控制模式
//! \param[in] regName   寄存器名称
//! \param[in] data      数据
//! \return    控制字
static inline DRV_Word_t DRV8316_buildCtrlWord(
                                            const DRV8316_CtrlMode_e ctrlMode,
                                            const DRV8316_Address_e regAddr,
                                            const uint16_t data)
{
    uint16_t p_addr = regAddr;
    uint16_t p_data = data;
    uint16_t p_mode = ctrlMode;

    uint16_t calc = (p_mode & 0x8000) | (p_addr & 0x7E00) | (p_data & 0x00FF);
    uint16_t parity = 0;
    while(calc)
    {
        parity ^= (calc & 1);
        calc >>= 1;
    }

    parity <<= 8;

    DRV_Word_t ctrlWord = ctrlMode | regAddr | parity | (data & DRV8316_DATA_MASK);

    return(ctrlWord);
} // DRV8316_buildCtrlWord() 函数结束

//! \brief     使能 DRV8316
//! \param[in] handle     DRV8316 句柄
extern void DRV8316_enable(DRV8316_Handle handle);

//! \brief     设置 DRV8316 的 SPI 句柄
//! \param[in] handle     DRV8316 句柄
//! \param[in] spiHandle  要使用的 SPI 句柄
void DRV8316_setSPIHandle(DRV8316_Handle handle,uint32_t spiHandle);

//! \brief     设置 DRV8316 的 GPIO 编号
//! \param[in] handle       DRV8316 句柄
//! \param[in] gpioHandle   要使用的 GPIO 编号
void DRV8316_setGPIOCSNumber(DRV8316_Handle handle,uint32_t gpioNumber);

//! \brief     Sets the GPIO number in the DRV8316
//! \param[in] handle       DRV8316 句柄
//! \param[in] gpioHandle   要使用的 GPIO 编号
void DRV8316_setGPIOENNumber(DRV8316_Handle handle,uint32_t gpioNumber);

//! \brief     Resets the enable timeout flag
//! \param[in] handle   DRV8316 句柄
static inline void DRV8316_resetEnableTimeout(DRV8316_Handle handle)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;

    obj->enableTimeOut = false;

    return;
} // DRV8316_resetEnableTimeout() 函数结束

//! \brief     重置 RX FIFO 超时标志
//! \param[in] handle   DRV8316 句柄
static inline void DRV8316_resetRxTimeout(DRV8316_Handle handle)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;

    obj->rxTimeOut = false;

    return;
} // DRV8316_resetRxTimeout() 函数结束

//! \brief     初始化所有 8320 SPI 变量的接口
//! \param[in] handle  DRV8316 句柄
extern void DRV8316_setupSPI(DRV8316_Handle handle,
                             DRV8316_VARS_t *drv8316Vars);

//! \brief     从 DRV8316 寄存器读取数据
//! \param[in] handle   DRV8316 句柄
//! \param[in] regAddr  寄存器地址
//! \return    数据值
extern uint16_t DRV8316_readSPI(DRV8316_Handle handle,
                                const DRV8316_Address_e regAddr);

//! \brief     向 DRV8316 寄存器写入数据
//! \param[in] handle   DRV8316 句柄
//! \param[in] regAddr  寄存器名称
//! \param[in] data     数据值
extern void DRV8316_writeSPI(DRV8316_Handle handle,
                             const DRV8316_Address_e regAddr,
                             const uint16_t data);

//! \brief     写入 DRV8316 SPI 寄存器
//! \param[in] handle  DRV8316 句柄
//! \param[in] drv8316Vars  包含 DRV8316 状态/控制寄存器选项的结构
//!                           所有 DRV8316 状态/控制寄存器选项
extern void DRV8316_writeData(DRV8316_Handle handle,
                              DRV8316_VARS_t *drv8316Vars);

//! \brief     读取 DRV8316 SPI 寄存器
//! \param[in] handle  DRV8316 句柄
//! \param[in] drv8316Vars  包含 DRV8316 状态/控制寄存器选项的结构
//!                           所有 DRV8316 状态/控制寄存器选项
extern void DRV8316_readData(DRV8316_Handle handle,
                             DRV8316_VARS_t *drv8316Vars);

//*****************************************************************************
//
// 关闭 Doxygen 分组。
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// 标记 C++ 编译器下 C 语言绑定区的结束。
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // DRV8316S_H 定义结束
