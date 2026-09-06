#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "app_core_dispatcher.h"
#include "app_core_runtime.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Runtime 运行上下文。
     *
     * Runtime 负责把底层硬件操作结果转换成 App Core Event。
     *
     * 当前版本用于验证 App Core 的完整调用链：
     *
     * Request
     *     -> Domain
     *     -> Controller
     *     -> Runtime
     *     -> Event
     *     -> State Store
     */

    typedef struct
    {
        /** App Core Dispatcher，用于发送 Event。 */
        app_core_dispatcher_t *dispatcher;

        /** 模拟图库照片总数。 */
        uint16_t gallery_total;

        /** 下一个模拟照片句柄。 */
        app_core_photo_handle_t next_photo_handle;

        /** Runtime 是否已经初始化。 */
        bool initialized;

    } app_runtime_context_t;
    /**
     * @brief 初始化 App Runtime。
     *
     * 函数会初始化 Runtime 回调表，
     * 并将所有底层上下文绑定到 app_runtime_context_t。
     *
     * @param runtime 要初始化的 Runtime 回调表。
     * @param context Runtime 上下文。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_runtime_init(
        app_core_runtime_t *runtime,
        app_runtime_context_t *context);
    // Your code here

#ifdef __cplusplus
}
#endif

#endif // APP_RUNTIME_H