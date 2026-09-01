#include <stddef.h>

#include "app_core_action.h"

/**
 * @brief 初始化 Action 对象。
 *
 * 初始化后 Action 处于 CREATED 状态，
 * 错误码默认为 ESP_OK。
 */
void app_core_action_init(
    app_core_action_t *action,
    app_core_action_id_t action_id,
    const app_core_effect_t *effect)
{
    if (action == NULL)
    {
        return;
    }

    *action = (app_core_action_t){
        .action_id = action_id,
        .state = APP_CORE_ACTION_STATE_CREATED,
        .last_error = ESP_OK,
    };

    if (effect != NULL)
    {
        action->effect = *effect;
    }
}

/**
 * @brief 检查 Action 状态是否有效。
 */
bool app_core_action_state_is_valid(
    app_core_action_state_t state)
{
    return state >= APP_CORE_ACTION_STATE_CREATED &&
           state <= APP_CORE_ACTION_STATE_CANCELED;
}

/**
 * @brief 检查 Action 对象是否有效。
 *
 * 检查内容：

 * 1. Action 指针不为空；
 * 2. Action 编号不是无效编号；
 * 3. Action 状态有效；
 * 4. Action 对应的 Effect 有效。
 */
bool app_core_action_is_valid(
    const app_core_action_t *action)
{
    if (action == NULL)
    {
        return false;
    }

    if (action->action_id ==
        APP_CORE_INVALID_ACTION_ID)
    {
        return false;
    }

    if (!app_core_action_state_is_valid(action->state))
    {
        return false;
    }

    return app_core_effect_is_valid(&action->effect);
}

/**
 * @brief 将 Action 状态转换成字符串。
 */
const char *app_core_action_state_to_string(
    app_core_action_state_t state)
{
    switch (state)
    {
    case APP_CORE_ACTION_STATE_CREATED:
        return "CREATED";

    case APP_CORE_ACTION_STATE_QUEUED:
        return "QUEUED";

    case APP_CORE_ACTION_STATE_EXECUTING:
        return "EXECUTING";

    case APP_CORE_ACTION_STATE_WAITING_RESULT:
        return "WAITING_RESULT";

    case APP_CORE_ACTION_STATE_SUCCEEDED:
        return "SUCCEEDED";

    case APP_CORE_ACTION_STATE_FAILED:
        return "FAILED";

    case APP_CORE_ACTION_STATE_CANCELED:
        return "CANCELED";

    default:
        return "UNKNOWN";
    }
}