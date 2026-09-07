#include "effect/app_core_storage_effect.h"

bool app_core_storage_effect_type_is_valid(
    app_core_storage_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_STORAGE_EFFECT_SAVE_PHOTO:
    case APP_CORE_STORAGE_EFFECT_SCAN_GALLERY:
    case APP_CORE_STORAGE_EFFECT_SHOW_GALLERY_PHOTO:
        return true;

    default:
        return false;
    }
}

const char *app_core_storage_effect_type_to_string(
    app_core_storage_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_STORAGE_EFFECT_NONE:
        return "NONE";

    case APP_CORE_STORAGE_EFFECT_SAVE_PHOTO:
        return "SAVE_PHOTO";

    case APP_CORE_STORAGE_EFFECT_SCAN_GALLERY:
        return "SCAN_GALLERY";

    case APP_CORE_STORAGE_EFFECT_SHOW_GALLERY_PHOTO:
        return "SHOW_GALLERY_PHOTO";

    default:
        return "UNKNOWN";
    }
}