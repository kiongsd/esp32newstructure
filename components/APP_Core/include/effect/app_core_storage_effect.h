#ifndef APP_CORE_STORAGE_EFFECT_H
#define APP_CORE_STORAGE_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_STORAGE_EFFECT_NONE = 0,

        /** 保存照片。 */
        APP_CORE_STORAGE_EFFECT_SAVE_PHOTO,

        /** 扫描图库。 */
        APP_CORE_STORAGE_EFFECT_SCAN_GALLERY,

        /** 显示图库中的指定照片。 */
        APP_CORE_STORAGE_EFFECT_SHOW_GALLERY_PHOTO,

    } app_core_storage_effect_type_t;

    bool app_core_storage_effect_type_is_valid(
        app_core_storage_effect_type_t type);

    const char *app_core_storage_effect_type_to_string(
        app_core_storage_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif