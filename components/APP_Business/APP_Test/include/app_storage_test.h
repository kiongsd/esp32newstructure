#ifndef APP_STORAGE_TEST_H
#define APP_STORAGE_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 执行 APP_Storage 集成测试。
 *
 * 测试内容包括：
 * 1. Storage Controller 初始化；
 * 2. Storage Domain 初始化和注册；
 * 3. SCAN_GALLERY 流程；
 * 4. SHOW_GALLERY_PHOTO 流程；
 * 5. Gallery 状态同步到公共 State Store。
 *
 * 测试使用假的 Runtime 回调，不依赖真实 SD 卡。
 */
void app_storage_test_run(void);

#ifdef __cplusplus
}
#endif

#endif
