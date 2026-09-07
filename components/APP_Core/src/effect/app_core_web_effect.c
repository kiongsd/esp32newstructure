#include "effect/app_core_web_effect.h"

bool app_core_web_effect_type_is_valid(
    app_core_web_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_WEB_EFFECT_START:
    case APP_CORE_WEB_EFFECT_STOP:
        return true;

    default:
        return false;
    }
}

const char *app_core_web_effect_type_to_string(
    app_core_web_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_WEB_EFFECT_NONE:
        return "NONE";

    case APP_CORE_WEB_EFFECT_START:
        return "WEB_START";

    case APP_CORE_WEB_EFFECT_STOP:
        return "WEB_STOP";

    default:
        return "UNKNOWN";
    }
}