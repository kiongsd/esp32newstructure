#ifndef APP_CORE_SYSTEM_EFFECT_H
#define APP_CORE_SYSTEM_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_SYSTEM_EFFECT_NONE = 0,

        /** 初始化系统。 */
        APP_CORE_SYSTEM_EFFECT_INITIALIZE,

        /** 挂起系统。 */
        APP_CORE_SYSTEM_EFFECT_SUSPEND,

        /** 恢复系统。 */
        APP_CORE_SYSTEM_EFFECT_RESUME,

    } app_core_system_effect_type_t;

    bool app_core_system_effect_type_is_valid(
        app_core_system_effect_type_t type);

    const char *app_core_system_effect_type_to_string(
        app_core_system_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif