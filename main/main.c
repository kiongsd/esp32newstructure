#include "app_system_test.h"
#include "app_capture_test.h"
#include "app_storage_test.h"

/**
 * @brief 应用层集成测试入口。
 *
 * 依次执行 System、Capture 和 Storage 业务测试。
 */
void app_main(void)
{
    app_system_test_run();
    app_capture_test_run();
    app_storage_test_run();
}
