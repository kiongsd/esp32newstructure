#ifndef APP_CORE_CAMERA_EFFECT_H
#define APP_CORE_CAMERA_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_CAMERA_EFFECT_NONE = 0,

        /** 启动 Camera 预览。 */
        APP_CORE_CAMERA_EFFECT_START_PREVIEW,

        /** 准备 Camera 拍照。 */
        APP_CORE_CAMERA_EFFECT_PREPARE_PHOTO,

        /** 停止 Camera。 */
        APP_CORE_CAMERA_EFFECT_STOP,

        /** 执行 Camera 拍照。 */
        APP_CORE_CAMERA_EFFECT_CAPTURE,

        /** 执行 Camera 自动对焦。 */
        APP_CORE_CAMERA_EFFECT_FOCUS,

    } app_core_camera_effect_type_t;

    bool app_core_camera_effect_type_is_valid(
        app_core_camera_effect_type_t type);

    const char *app_core_camera_effect_type_to_string(
        app_core_camera_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif