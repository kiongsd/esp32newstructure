#include "effect/app_core_camera_effect.h"

bool app_core_camera_effect_type_is_valid(
    app_core_camera_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_CAMERA_EFFECT_START_PREVIEW:
    case APP_CORE_CAMERA_EFFECT_PREPARE_PHOTO:
    case APP_CORE_CAMERA_EFFECT_STOP:
    case APP_CORE_CAMERA_EFFECT_CAPTURE:
    case APP_CORE_CAMERA_EFFECT_FOCUS:
        return true;

    default:
        return false;
    }
}

const char *app_core_camera_effect_type_to_string(
    app_core_camera_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_CAMERA_EFFECT_NONE:
        return "NONE";

    case APP_CORE_CAMERA_EFFECT_START_PREVIEW:
        return "CAMERA_START_PREVIEW";

    case APP_CORE_CAMERA_EFFECT_PREPARE_PHOTO:
        return "CAMERA_PREPARE_PHOTO";

    case APP_CORE_CAMERA_EFFECT_STOP:
        return "CAMERA_STOP";

    case APP_CORE_CAMERA_EFFECT_CAPTURE:
        return "CAMERA_CAPTURE";

    case APP_CORE_CAMERA_EFFECT_FOCUS:
        return "CAMERA_FOCUS";

    default:
        return "UNKNOWN";
    }
}