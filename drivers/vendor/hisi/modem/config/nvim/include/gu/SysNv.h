

#ifndef __SYS_NV_H__
#define __SYS_NV_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "NvIddef.h"

/****************************************************************************
  2 宏定义声明
*****************************************************************************/

#define SYS_APP_NV_CFG_LEN              (sizeof(VOS_UINT16))


/****************************************************************************
  3 枚举声明
*****************************************************************************/

enum ENUM_SYSTEM_APP_CONFIG_TYPE
{
    SYSTEM_APP_MP       = 0,    /*上层应用是Mobile Partner*/
    SYSTEM_APP_WEBUI,           /*上层应用是Web UI*/
    SYSTEM_APP_ANDROID,         /*上层应用是Android*/
    SYSTEM_APP_BUTT
};
typedef VOS_UINT16 ENUM_SYSTEM_APP_CONFIG_TYPE_U16 ;

/****************************************************************************
  4 结构体声明
*****************************************************************************/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* __SYSNVID_H__ */



