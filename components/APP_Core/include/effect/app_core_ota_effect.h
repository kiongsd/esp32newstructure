#ifndef APP_CORE_OTA_EFFECT_H
#define APP_CORE_OTA_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_OTA_EFFECT_NONE = 0,

        /** 开始 OTA。 */
        APP_CORE_OTA_EFFECT_BEGIN,

        /** 完成 OTA。 */
        APP_CORE_OTA_EFFECT_FINISH,

        /** 中止 OTA。 */
        APP_CORE_OTA_EFFECT_ABORT,

    } app_core_ota_effect_type_t;

    bool app_core_ota_effect_type_is_valid(
        app_core_ota_effect_type_t type);

    const char *app_core_ota_effect_type_to_string(
        app_core_ota_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif