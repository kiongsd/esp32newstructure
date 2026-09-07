#include "effect/app_core_ui_effect.h"

bool app_core_ui_effect_type_is_valid(
    app_core_ui_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_UI_EFFECT_SHOW_PAGE:
    case APP_CORE_UI_EFFECT_UPDATE_MENU_SELECTION:
        return true;

    default:
        return false;
    }
}

const char *app_core_ui_effect_type_to_string(
    app_core_ui_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_UI_EFFECT_NONE:
        return "NONE";

    case APP_CORE_UI_EFFECT_SHOW_PAGE:
        return "SHOW_PAGE";

    case APP_CORE_UI_EFFECT_UPDATE_MENU_SELECTION:
        return "UPDATE_MENU_SELECTION";

    default:
        return "UNKNOWN";
    }
}