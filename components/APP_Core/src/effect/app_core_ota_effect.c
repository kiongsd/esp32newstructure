#include "effect/app_core_ota_effect.h"

bool app_core_ota_effect_type_is_valid(
    app_core_ota_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_OTA_EFFECT_BEGIN:
    case APP_CORE_OTA_EFFECT_FINISH:
    case APP_CORE_OTA_EFFECT_ABORT:
        return true;

    default:
        return false;
    }
}

const char *app_core_ota_effect_type_to_string(
    app_core_ota_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_OTA_EFFECT_NONE:
        return "NONE";

    case APP_CORE_OTA_EFFECT_BEGIN:
        return "OTA_BEGIN";

    case APP_CORE_OTA_EFFECT_FINISH:
        return "OTA_FINISH";

    case APP_CORE_OTA_EFFECT_ABORT:
        return "OTA_ABORT";

    default:
        return "UNKNOWN";
    }
}