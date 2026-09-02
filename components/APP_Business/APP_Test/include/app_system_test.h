#ifndef APP_SYSTEM_TEST_H
#define APP_SYSTEM_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 执行 APP_System 集成测试。
     *
     * 测试内容包括：
     *
     * 1. System Controller 初始化；
     * 2. System Domain 初始化；
     * 3. System Domain 注册到 Dispatcher；
     * 4. SYSTEM_INITIALIZE 流程；
     * 5. SYSTEM_SUSPEND 流程；
     * 6. SYSTEM_RESUME 流程。
     *
     * 当前使用假的 Runtime 回调，
     * 不依赖真实开发板和硬件驱动。
     */
    void app_system_test_run(void);

#ifdef __cplusplus
}
#endif

#endif