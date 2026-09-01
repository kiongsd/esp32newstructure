#ifndef APP_CORE_STATE_STORE_H
#define APP_CORE_STATE_STORE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "app_core_state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 状态存储对象。
     *
     * 该对象统一管理状态快照，
     * 并通过互斥锁保证多个任务访问状态时不会发生冲突。
     */
    typedef struct
    {
        /** 保护状态快照读写的互斥锁。 */
        SemaphoreHandle_t mutex;

        /** 当前保存的状态快照。 */
        app_core_state_snapshot_t snapshot;

        /** State Store 是否已经初始化。 */
        bool initialized;

    } app_core_state_store_t;

    /**
     * @brief 初始化 State Store。
     *
     * 创建互斥锁，并设置默认状态。
     *
     * @param store 要初始化的状态存储对象。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_state_store_init(
        app_core_state_store_t *store);

    /**
     * @brief 释放 State Store 资源。
     *
     * 删除互斥锁，并清空状态快照。
     *
     * @param store 要释放的状态存储对象。
     */
    void app_core_state_store_deinit(
        app_core_state_store_t *store);

    /**
     * @brief 读取当前状态快照。
     *
     * 函数会复制一份状态快照给调用者，
     * 调用者可以在锁外读取这份副本。
     *
     * @param store 状态存储对象。
     * @param snapshot 用于接收状态快照的对象。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_state_store_read(
        const app_core_state_store_t *store,
        app_core_state_snapshot_t *snapshot);

    /**
     * @brief 发布新的状态快照。
     *
     * 如果新状态与旧状态不同，
     * revision 会自动递增。
     *
     * @param store 状态存储对象。
     * @param snapshot 要发布的新状态。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_state_store_publish(
        app_core_state_store_t *store,
        const app_core_state_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif