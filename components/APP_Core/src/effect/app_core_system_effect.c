#include "effect/app_core_system_effect.h"

bool app_core_system_effect_type_is_valid(
    app_core_system_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_SYSTEM_EFFECT_INITIALIZE:
    case APP_CORE_SYSTEM_EFFECT_SUSPEND:
    case APP_CORE_SYSTEM_EFFECT_RESUME:
        return true;

    default:
        return false;
    }
}

const char *app_core_system_effect_type_to_string(
    app_core_system_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_SYSTEM_EFFECT_NONE:
        return "NONE";

    case APP_CORE_SYSTEM_EFFECT_INITIALIZE:
        return "SYSTEM_INITIALIZE";

    case APP_CORE_SYSTEM_EFFECT_SUSPEND:
        return "SYSTEM_SUSPEND";

    case APP_CORE_SYSTEM_EFFECT_RESUME:
        return "SYSTEM_RESUME";

    default:
        return "UNKNOWN";
    }
}