#ifndef APP_UI_TEST_H
#define APP_UI_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 执行 APP_UI 集成测试。
 *
 * 测试内容包括：
 * 1. UI Controller 初始化；
 * 2. UI Domain 初始化和注册；
 * 3. SHOW_PAGE 请求切换到主菜单页面；
 * 4. SHOW_PAGE 请求切换到设置页面；
 * 5. 页面状态同步到公共 State Store。
 *
 * 测试使用假的 Runtime 回调，不依赖真实 LVGL 显示设备。
 */
void app_ui_test_run(void);

#ifdef __cplusplus
}
#endif

#endif
