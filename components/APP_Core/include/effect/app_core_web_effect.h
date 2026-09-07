#ifndef APP_CORE_WEB_EFFECT_H
#define APP_CORE_WEB_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_WEB_EFFECT_NONE = 0,

        /** 启动 Web 服务。 */
        APP_CORE_WEB_EFFECT_START,

        /** 停止 Web 服务。 */
        APP_CORE_WEB_EFFECT_STOP,

    } app_core_web_effect_type_t;

    bool app_core_web_effect_type_is_valid(
        app_core_web_effect_type_t type);

    const char *app_core_web_effect_type_to_string(
        app_core_web_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif