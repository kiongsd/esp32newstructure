#ifndef APP_CAPTURE_TEST_H
#define APP_CAPTURE_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 执行 APP_Capture 集成测试。
 *
 * 测试内容包括：
 * 1. 启动 Camera 预览；
 * 2. 准备拍照；
 * 3. 执行完整拍照并保存照片；
 * 4. 重新启动预览并测试对焦；
 * 5. 停止 Camera 预览。
 *
 * 测试使用假的 Runtime 回调，不依赖真实 Camera。
 */
void app_capture_test_run(void);

#ifdef __cplusplus
}
#endif

#endif
