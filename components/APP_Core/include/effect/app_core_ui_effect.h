#ifndef APP_CORE_UI_EFFECT_H
#define APP_CORE_UI_EFFECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_CORE_UI_EFFECT_NONE = 0,

        /** 显示指定页面。 */
        APP_CORE_UI_EFFECT_SHOW_PAGE,

        /** 更新菜单选择项。 */
        APP_CORE_UI_EFFECT_UPDATE_MENU_SELECTION,

    } app_core_ui_effect_type_t;

    bool app_core_ui_effect_type_is_valid(
        app_core_ui_effect_type_t type);

    const char *app_core_ui_effect_type_to_string(
        app_core_ui_effect_type_t type);

#ifdef __cplusplus
}
#endif

#endif