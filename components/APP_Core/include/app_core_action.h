#ifndef APP_CORE_ACTION_H
#define APP_CORE_ACTION_H

#include "app_core_effect.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Action 编号。
     *
     * 每个 Action 都应该有唯一编号，
     * 用于追踪动作的执行过程。
     */
    typedef uint32_t app_core_action_id_t;

/**
 * @brief 无效 Action 编号。
 */
#define APP_CORE_INVALID_ACTION_ID ((app_core_action_id_t)0U)

    /**
     * @brief Action 执行状态。
     */
    typedef enum
    {
        /** Action 已创建，但尚未提交。 */
        APP_CORE_ACTION_STATE_CREATED = 0,

        /** Action 已经进入等待执行队列。 */
        APP_CORE_ACTION_STATE_QUEUED,

        /** Action 正在执行。 */
        APP_CORE_ACTION_STATE_EXECUTING,

        /** Action 已经发起，正在等待底层结果。 */
        APP_CORE_ACTION_STATE_WAITING_RESULT,

        /** Action 执行成功。 */
        APP_CORE_ACTION_STATE_SUCCEEDED,

        /** Action 执行失败。 */
        APP_CORE_ACTION_STATE_FAILED,

        /** Action 已被取消。 */
        APP_CORE_ACTION_STATE_CANCELED,

    } app_core_action_state_t;

    /**
     * @brief App Core Action 对象。
     *
     * Action 用于记录一个 Effect 的执行状态。
     */
    typedef struct
    {
        /** Action 的唯一编号。 */
        app_core_action_id_t action_id;

        /** Action 当前执行状态。 */
        app_core_action_state_t state;

        /** Action 对应的具体 Effect。 */
        app_core_effect_t effect;

        /** Action 最近一次执行错误。 */
        esp_err_t last_error;

    } app_core_action_t;

    /**
     * @brief 初始化 Action 对象。
     *
     * Action 初始化后默认处于 CREATED 状态。
     *
     * @param action 要初始化的 Action。
     * @param action_id Action 编号。
     * @param effect Action 对应的 Effect。
     */
    void app_core_action_init(
        app_core_action_t *action,
        app_core_action_id_t action_id,
        const app_core_effect_t *effect);

    /**
     * @brief 检查 Action 状态是否有效。
     *
     * @param state 要检查的 Action 状态。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_action_state_is_valid(
        app_core_action_state_t state);

    /**
     * @brief 检查 Action 对象是否有效。
     *
     * @param action 要检查的 Action。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_action_is_valid(
        const app_core_action_t *action);

    /**
     * @brief 将 Action 状态转换成字符串。
     *
     * 主要用于日志和调试。
     *
     * @param state Action 状态。
     * @return Action 状态对应的字符串。
     */
    const char *app_core_action_state_to_string(
        app_core_action_state_t state);

#ifdef __cplusplus
}
#endif

#endif