#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "app_core_request_gateway.h"

/**
 * @brief 当前 Request 提交回调。
 */
static app_core_request_gateway_submit_fn s_submit;

/**
 * @brief Request 提交回调的上下文。
 */
static void *s_submit_ctx;

/**
 * @brief 当前状态读取回调。
 */
static app_core_request_gateway_read_state_fn s_read_state;

/**
 * @brief 状态读取回调的上下文。
 */
static void *s_read_state_ctx;

/**
 * @brief 下一个可分配的 Request ID。
 */
static app_core_request_id_t s_next_request_id = 1U;

/**
 * @brief Gateway 是否已经初始化。
 */
static bool s_initialized;

/**
 * @brief 保护 Gateway 全局变量的临界区锁。
 */
static portMUX_TYPE s_gateway_mux =
    portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief 在临界区内分配新的 Request ID。
 *
 * Request ID 用完溢出后重新从 1 开始，
 * 0 始终保留给无效 Request ID。
 */
static app_core_request_id_t
allocate_request_id_locked(void)
{
    app_core_request_id_t request_id;

    if (s_next_request_id ==
        APP_CORE_INVALID_REQUEST_ID)
    {
        s_next_request_id = 1U;
    }

    request_id = s_next_request_id;
    s_next_request_id++;

    return request_id;
}

/**
 * @brief 初始化 Request Gateway。
 */
esp_err_t app_core_request_gateway_init(void)
{
    portENTER_CRITICAL(&s_gateway_mux);

    if (s_initialized)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return ESP_ERR_INVALID_STATE;
    }

    s_submit = NULL;
    s_submit_ctx = NULL;
    s_read_state = NULL;
    s_read_state_ctx = NULL;
    s_next_request_id = 1U;
    s_initialized = true;

    portEXIT_CRITICAL(&s_gateway_mux);

    return ESP_OK;
}

/**
 * @brief 释放 Request Gateway。
 */
void app_core_request_gateway_deinit(
    void *submit_ctx)
{
    portENTER_CRITICAL(&s_gateway_mux);

    if (submit_ctx == NULL ||
        s_submit_ctx == NULL ||
        s_submit_ctx == submit_ctx)
    {
        s_submit = NULL;
        s_submit_ctx = NULL;
        s_read_state = NULL;
        s_read_state_ctx = NULL;
        s_next_request_id = 1U;
        s_initialized = false;
    }

    portEXIT_CRITICAL(&s_gateway_mux);
}

/**
 * @brief 绑定 Request 提交回调和状态读取回调。
 */
esp_err_t app_core_request_gateway_bind(
    app_core_request_gateway_submit_fn submit,
    void *submit_ctx,
    app_core_request_gateway_read_state_fn read_state,
    void *read_state_ctx)
{
    if (submit == NULL ||
        submit_ctx == NULL ||
        read_state == NULL ||
        read_state_ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_gateway_mux);

    if (!s_initialized)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return ESP_ERR_INVALID_STATE;
    }

    if (s_submit != NULL)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return ESP_ERR_INVALID_STATE;
    }

    s_submit = submit;
    s_submit_ctx = submit_ctx;
    s_read_state = read_state;
    s_read_state_ctx = read_state_ctx;

    portEXIT_CRITICAL(&s_gateway_mux);

    return ESP_OK;
}

/**
 * @brief 解除 Request Gateway 绑定。
 */
void app_core_request_gateway_unbind(
    void *submit_ctx)
{
    portENTER_CRITICAL(&s_gateway_mux);

    if (submit_ctx == NULL ||
        s_submit_ctx == submit_ctx)
    {
        s_submit = NULL;
        s_submit_ctx = NULL;
        s_read_state = NULL;
        s_read_state_ctx = NULL;
    }

    portEXIT_CRITICAL(&s_gateway_mux);
}

/**
 * @brief 提交 Request。
 *
 * 先复制 Request，再在副本上分配 Request ID，
 * 避免修改调用者传入的原始对象。
 */
esp_err_t app_core_request_gateway_submit(
    const app_core_request_t *request)
{
    app_core_request_t queued_request;
    app_core_request_gateway_submit_fn submit;
    void *submit_ctx;

    if (request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (request->type ==
        APP_CORE_REQUEST_TYPE_NONE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    queued_request = *request;

    portENTER_CRITICAL(&s_gateway_mux);

    if (!s_initialized ||
        s_submit == NULL)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return ESP_ERR_INVALID_STATE;
    }

    if (queued_request.meta.request_id ==
        APP_CORE_INVALID_REQUEST_ID)
    {
        queued_request.meta.request_id =
            allocate_request_id_locked();
    }

    submit = s_submit;
    submit_ctx = s_submit_ctx;

    portEXIT_CRITICAL(&s_gateway_mux);

    return submit(
        submit_ctx,
        &queued_request);
}

/**
 * @brief 读取当前状态。
 */
esp_err_t app_core_request_gateway_read_state(
    app_core_state_snapshot_t *snapshot)
{
    app_core_request_gateway_read_state_fn read_state;
    void *read_state_ctx;

    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_gateway_mux);

    if (!s_initialized ||
        s_read_state == NULL)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return ESP_ERR_INVALID_STATE;
    }

    read_state = s_read_state;
    read_state_ctx = s_read_state_ctx;

    portEXIT_CRITICAL(&s_gateway_mux);

    return read_state(
        read_state_ctx,
        snapshot);
}

/**
 * @brief 获取下一个 Request ID。
 */
app_core_request_id_t
app_core_request_gateway_next_request_id(void)
{
    app_core_request_id_t request_id;

    portENTER_CRITICAL(&s_gateway_mux);

    if (!s_initialized)
    {
        portEXIT_CRITICAL(&s_gateway_mux);
        return APP_CORE_INVALID_REQUEST_ID;
    }

    request_id = allocate_request_id_locked();

    portEXIT_CRITICAL(&s_gateway_mux);

    return request_id;
}