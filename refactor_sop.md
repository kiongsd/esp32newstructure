
# C:\99_lvgl 后续重构 SOP

## 1. 文档目的

本文档以当前工程 C:\99_lvgl 的实际结构为基线，指导后续手动重构。

当前没有开发板，因此每个阶段暂时以以下结果作为成功标准：

1. 新增文件已经放入正确目录。
2. CMake 能够找到新增目录和源文件。
3. 头文件声明与源文件实现一致。
4. 没有编译错误。
5. 没有链接错误。
6. 最终能够生成工程 ELF、BIN 等构建产物。
7. 当前保留假 Runtime、假事件回调和测试代码，等拿到开发板后再做真实硬件验证。

本文档只提供后续操作方案，不自动修改现有源码。

---

## 2. 当前工程基线

### 2.1 当前目录结构

当前工程结构如下：

~~~text
C:\99_lvgl
├── CMakeLists.txt
├── sdkconfig
├── sdkconfig.defaults
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── components
    ├── APP_Core
    │   ├── CMakeLists.txt
    │   ├── include
    │   │   ├── app_core_action.h
    │   │   ├── app_core_action_engine.h
    │   │   ├── app_core_dispatcher.h
    │   │   ├── app_core_domain.h
    │   │   ├── app_core_effect.h
    │   │   ├── app_core_event.h
    │   │   ├── app_core_request.h
    │   │   ├── app_core_request_gateway.h
    │   │   ├── app_core_runtime.h
    │   │   ├── app_core_state.h
    │   │   ├── app_core_state_store.h
    │   │   ├── app_core_task.h
    │   │   └── app_core_types.h
    │   └── src
    │       ├── app_core_action.c
    │       ├── app_core_action_engine.c
    │       ├── app_core_dispatcher.c
    │       ├── app_core_domain.c
    │       ├── app_core_effect.c
    │       ├── app_core_event.c
    │       ├── app_core_request.c
    │       ├── app_core_request_gateway.c
    │       ├── app_core_runtime.c
    │       ├── app_core_state.c
    │       ├── app_core_state_store.c
    │       ├── app_core_task.c
    │       └── app_core_types.c
    └── APP_Business
        ├── CMakeLists.txt
        ├── APP_System
        │   ├── include
        │   │   ├── app_system_controller.h
        │   │   ├── app_system_domain.h
        │   │   └── app_system_fsm.h
        │   └── src
        │       ├── app_system_controller.c
        │       ├── app_system_domain.c
        │       └── app_system_fsm.c
        └── APP_Test
            ├── include
            │   └── app_system_test.h
            └── src
                └── app_system_test.c
~~~

APP_Core 的公共接口已经建立并暂时冻结。后续迁移 Capture 时，不要同时修改 APP_Core。

### 2.2 当前层次职责

APP_Core 是公共层，负责：

- 基础类型、枚举和错误码。
- Request 请求协议。
- Event 事件协议。
- Effect 动作意图。
- Action 动作对象。
- State 系统状态。
- State Store 状态存储。
- Domain 通用领域接口。
- Dispatcher 分发。
- Request Gateway 请求入口。
- Runtime 运行时回调抽象。
- Action Engine 动作执行。
- App Task 任务调度。

APP_Business 是业务层，当前包含：

- APP_System：系统状态机、系统 Controller、系统 Domain。
- APP_Test：没有开发板时使用的假 Runtime 和编译期测试。

后续业务模块继续放在 APP_Business 下：

- APP_Capture
- APP_Storage
- APP_UI
- APP_Web
- APP_OTA

真实摄像头、SD 卡、LVGL、网络和 OTA 实现，后续放到 APP_Service、APP_Driver、BSP 或 Middlewares，不要直接写进 APP_Core。

### 2.3 当前根目录 CMakeLists.txt

文件：

~~~text
C:\99_lvgl\CMakeLists.txt
~~~

当前职责是加载 ESP-IDF 工程：

~~~cmake
cmake_minimum_required(VERSION 3.16)
add_compile_options(-fdiagnostics-color=always)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(25_1_camera)
~~~

后续业务迁移阶段通常不需要修改这个文件。不要在这里逐个添加业务源文件。

### 2.4 当前 main/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\main\CMakeLists.txt
~~~

当前应保持为：

~~~cmake
idf_component_register(
    SRCS
        "main.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        APP_Core
        APP_Business
)
~~~

规则：

- main/CMakeLists.txt 只编译 main.c。
- 不要把 APP_System/src、APP_Test/src、APP_Capture/src 等源文件再次写到这里。
- 所有业务源文件由 APP_Business/CMakeLists.txt 收集。
- 同一个 .c 文件只能由一个 ESP-IDF 组件编译。

### 2.5 当前 APP_Core/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\components\APP_Core\CMakeLists.txt
~~~

当前采用目录收集：

~~~cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

set(requires
    freertos
    esp_common
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

后续新增公共层文件时，只要放入 APP_Core/src 和 APP_Core/include，一般不需要逐个修改 CMakeLists.txt。

### 2.6 当前 APP_Business/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\components\APP_Business\CMakeLists.txt
~~~

当前应类似于：

~~~cmake
set(src_dirs
    APP_System/src
    APP_Test/src
)

set(include_dirs
    APP_System/include
    APP_Test/include
)

set(requires
    APP_Core
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

APP_System、APP_Test 是 APP_Business 的子目录，不是独立 ESP-IDF 组件。它们由 APP_Business/CMakeLists.txt 统一管理。

---

## 3. 每次迁移必须遵守的固定流程

每个业务模块都按照下面的顺序推进：

1. 建立模块的 include 和 src 目录。
2. 创建第一个功能的 .h 文件。
3. 紧接着创建对应的 .c 文件。
4. 在 .h 中写结构体、枚举、函数声明和注释。
5. 在 .c 中实现函数，并给每个函数写作用注释。
6. 修改 APP_Business/CMakeLists.txt。
7. 先不要修改 main.c。
8. 执行重新配置和编译。
9. 确认成功后再建立该模块的下一个功能。
10. FSM、Controller、Domain 全部能编译后，再建立测试。
11. 测试文件能编译后，最后才把测试入口加入 main.c。
12. 每次保留上一个阶段的可编译状态。

命令：

~~~powershell
cd C:\99_lvgl
idf.py reconfigure
idf.py build
~~~

只修改 .c 或 .h 时，通常执行：

~~~powershell
cd C:\99_lvgl
idf.py build
~~~

修改 CMakeLists.txt、增加目录或增加源文件后，执行：

~~~powershell
cd C:\99_lvgl
idf.py reconfigure
idf.py build
~~~

---

# 4. 阶段一：迁移 APP_Capture 状态机

当前 System 业务已经完成，下一步从 Capture FSM 开始。

## 4.1 新建目录

建立：

~~~text
C:\99_lvgl\components\APP_Business\APP_Capture
├── include
└── src
~~~

本阶段不要修改 APP_Core、根目录 CMakeLists.txt 或 main.c。

## 4.2 创建 app_capture_fsm.h

新建：

~~~text
C:\99_lvgl\components\APP_Business\APP_Capture\include\app_capture_fsm.h
~~~

这个头文件负责定义：

- Capture 状态枚举。
- Capture 输入事件枚举。
- Capture 输出动作枚举。
- Capture FSM 上下文结构体。
- FSM 初始化接口。
- Request/Event 处理接口。
- 状态查询接口。
- 状态转字符串接口。

第一版建议包含：

~~~c
APP_CAPTURE_STATE_UNKNOWN
APP_CAPTURE_STATE_IDLE
APP_CAPTURE_STATE_PREPARING
APP_CAPTURE_STATE_ACQUIRING
APP_CAPTURE_STATE_PROCESSING
APP_CAPTURE_STATE_SAVING
APP_CAPTURE_STATE_SUCCEEDED
APP_CAPTURE_STATE_FAILED
~~~

建议流程：

~~~text
IDLE
  └─ CAPTURE_START
       ↓
PREPARING
  └─ CAPTURE_PREPARED
       ↓
ACQUIRING
  └─ FRAME_CAPTURED
       ↓
PROCESSING
  └─ FRAME_PROCESSED
       ↓
SAVING
  └─ PHOTO_SAVED
       ↓
SUCCEEDED
  └─ CAPTURE_RESET
       ↓
IDLE
~~~

在头文件中为所有枚举、结构体和函数写说明，说明成员含义、读写方以及是否属于公共协议。

## 4.3 创建 app_capture_fsm.c

新建：

~~~text
C:\99_lvgl\components\APP_Business\APP_Capture\src\app_capture_fsm.c
~~~

实现：

- app_capture_fsm_init
- app_capture_fsm_handle_request
- app_capture_fsm_handle_event
- app_capture_fsm_get_state
- app_capture_fsm_state_to_string

每个函数前写明作用，例如：

~~~c
/*
 * 函数作用：
 * 初始化 Capture 状态机上下文。
 *
 * 处理内容：
 * 1. 设置初始状态。
 * 2. 清空当前照片句柄。
 * 3. 清除错误码和临时结果。
 * 4. 将内部计数器设置为默认值。
 */
~~~

第一版只实现状态转换和动作意图，不实现真实摄像头操作。

## 4.4 修改 APP_Business/CMakeLists.txt

将 src_dirs 修改为：

~~~cmake
set(src_dirs
    APP_System/src
    APP_Capture/src
    APP_Test/src
)
~~~

将 include_dirs 修改为：

~~~cmake
set(include_dirs
    APP_System/include
    APP_Capture/include
    APP_Test/include
)
~~~

requires 仍然是：

~~~cmake
set(requires
    APP_Core
)
~~~

main/CMakeLists.txt 和 main.c 不修改。

## 4.5 阶段一正确现象

执行：

~~~powershell
cd C:\99_lvgl
idf.py reconfigure
idf.py build
~~~

正确现象：

- CMake 重新配置成功。
- app_capture_fsm.c 被编译。
- 没有 Cannot find source file。
- 没有 No SOURCES given to target。
- 没有头文件找不到错误。
- 没有 undefined reference。
- 最终生成 ELF。
- 原来的 System 测试仍然可以编译。

如果出现链接错误，检查：

1. app_capture_fsm.c 是否在 APP_Capture/src。
2. APP_Capture/src 是否加入 APP_Business/CMakeLists.txt。
3. 头文件声明和源文件定义的函数名称是否一致。
4. 是否只有声明没有实现。
5. 是否把相同源文件重复加入 main/CMakeLists.txt。

---

# 5. 阶段二：迁移 APP_Capture Controller

FSM 单独编译成功后，建立 Controller。

## 5.1 新建文件

~~~text
C:\99_lvgl\components\APP_Business\APP_Capture
├── include
│   ├── app_capture_fsm.h
│   └── app_capture_controller.h
└── src
    ├── app_capture_fsm.c
    └── app_capture_controller.c
~~~

## 5.2 app_capture_controller.h 的职责

定义：

- Controller 上下文结构体。
- Controller 初始化函数。
- Request 处理函数。
- Event 处理函数。
- Action 执行函数。
- process_once 函数。
- 当前状态查询函数。

Controller 的关系：

~~~text
Capture Domain
      ↓ Request/Event
Capture Controller
      ↓
Capture FSM + APP_Core Action Engine
      ↓
Action
      ↓
Capture Runtime 或假 Runtime
~~~

Controller 不判断请求是否属于 Capture，这个判断由 Capture Domain 完成。

## 5.3 app_capture_controller.c 的职责

实现：

1. 初始化 Capture FSM。
2. 初始化 Action Queue 或 Action Engine。
3. 保存 Event Sink。
4. 接收 Domain 转交的 Request/Event。
5. 调用 FSM。
6. 将 FSM 产生的 Action 交给公共层执行器。
7. 提供 process_once，让外层执行一次动作。

第一版可以使用测试回调，不连接真实摄像头。

## 5.4 CMakeLists.txt 和 main.c

本阶段不需要再次修改 CMakeLists.txt。

本阶段不修改 main.c。Controller 先独立编译，避免把未完成的 Capture 集成到主流程。

## 5.5 阶段二正确现象

执行 idf.py build 后：

- app_capture_controller.c 被编译。
- System 现有测试不受影响。
- 没有 implicit declaration。
- 没有 undefined reference to app_capture_controller。
- 最终 ELF 链接成功。

---

# 6. 阶段三：迁移 APP_Capture Domain

Controller 编译成功后，建立 Domain。

## 6.1 新建文件

~~~text
C:\99_lvgl\components\APP_Business\APP_Capture
├── include
│   ├── app_capture_fsm.h
│   ├── app_capture_controller.h
│   └── app_capture_domain.h
└── src
    ├── app_capture_fsm.c
    ├── app_capture_controller.c
    └── app_capture_domain.c
~~~

## 6.2 app_capture_domain.h 的职责

声明：

- Capture Domain 上下文。
- Request 匹配函数。
- Event 匹配函数。
- Request 处理函数。
- Event 处理函数。
- State Store 同步函数。
- 获取 Domain Handler 的函数。

Domain 需要判断：

~~~text
请求是否属于 Capture？
事件是否属于 Capture？
当前状态是否允许这个请求？
请求应交给哪个 Controller？
事件如何更新 State Store？
~~~

## 6.3 app_capture_domain.c 的职责

实现：

1. 判断请求类型是不是 Capture 请求。
2. 判断事件类型是不是 Capture 事件。
3. 将合法请求交给 Capture Controller。
4. 将合法事件交给 Capture Controller。
5. 将 Capture 状态同步到公共 State Store。
6. 根据 Action Engine 执行结果处理后续 Event。
7. 对非法请求返回错误结果。

Domain 不直接操作摄像头、SD 卡、网络、LVGL 控件或 GPIO。

## 6.4 CMakeLists.txt 和 main.c

本阶段不需要再次修改 CMakeLists.txt。

本阶段不修改 main.c。

## 6.5 阶段三正确现象

- app_capture_domain.c 被编译。
- APP_Business 能够正确链接 APP_Core。
- 没有公共层函数的 undefined reference。
- System 代码和 System 测试仍可编译。
- 最终生成 ELF。

---

# 7. 阶段四：增加 Capture 假 Runtime 测试

FSM、Controller、Domain 全部编译成功后，再增加 Capture 测试。

## 7.1 新建文件

~~~text
C:\99_lvgl\components\APP_Business\APP_Test
├── include
│   ├── app_system_test.h
│   └── app_capture_test.h
└── src
    ├── app_system_test.c
    └── app_capture_test.c
~~~

## 7.2 app_capture_test.h

声明：

~~~c
void app_capture_test_run(void);
~~~

注释中说明：

- 这是无开发板条件下的假 Runtime 测试。
- 它验证公共协议、业务流程和链接关系。
- 它不代表真实摄像头已经工作。

## 7.3 app_capture_test.c

建立：

- State Store。
- Dispatcher。
- Capture Controller。
- Capture Domain。
- 假 Runtime 回调。
- 假 Event Sink。
- Request、Event 和 State 对象。

建议测试流程：

~~~text
1. 初始化 State Store
2. 初始化 Dispatcher
3. 初始化 Capture Controller
4. 初始化 Capture Domain
5. 注册 Capture Domain
6. 发送 CAPTURE_START 请求
7. 假 Runtime 产生 CAPTURE_PREPARED
8. 假 Runtime 产生 FRAME_CAPTURED
9. 假 Runtime 产生 FRAME_PROCESSED
10. 假 Runtime 产生 PHOTO_SAVED
11. 检查 Capture 状态为 SUCCEEDED
12. 检查照片句柄或结果字段已经同步
~~~

假回调要写清楚，例如：

~~~c
/*
 * 假 Runtime 回调。
 * 当前没有连接摄像头，只模拟“准备完成”事件。
 * 后续连接真实硬件时，再替换为 APP_Service 的真实实现。
 */
~~~

## 7.4 CMakeLists.txt 修改

不需要修改。APP_Test/src 和 APP_Test/include 已经加入 APP_Business/CMakeLists.txt。

## 7.5 main.c 修改

当前 main.c 已经包含 app_system_test.h，并在 app_main 中调用 app_system_test_run。当前版本中这些内容大约位于：

- include 区域的第 12 行附近。
- app_main 测试调用区域的大约第 181 行附近。

实际修改时搜索 app_system_test.h 和 app_system_test_run，不要机械依赖行号。

增加：

~~~c
#include "app_capture_test.h"
~~~

在 System 测试调用之后增加：

~~~c
app_capture_test_run();
~~~

建议顺序：

~~~c
app_system_test_run();
app_capture_test_run();
~~~

不要删除 System 测试。

## 7.6 阶段四正确现象

- app_capture_test.c 能够编译。
- main.c 能够找到 app_capture_test.h。
- 没有 undefined reference to app_capture_test_run。
- 最终链接成功。
- 当前没有开发板，因此不要求摄像头、屏幕或串口运行结果。
- 如果后续有日志，应该能看到 System 测试和 Capture 测试依次执行。

---

# 8. 阶段五：迁移 APP_Storage

Capture 独立测试成功后，再迁移 Storage。

## 8.1 新建目录和文件顺序

目录：

~~~text
C:\99_lvgl\components\APP_Business\APP_Storage
├── include
└── src
~~~

按以下顺序建立，每个 .h 和 .c 成对建立：

~~~text
1. app_storage_fsm.h
2. app_storage_fsm.c
3. 编译
4. app_storage_controller.h
5. app_storage_controller.c
6. 编译
7. app_storage_domain.h
8. app_storage_domain.c
9. 编译
~~~

## 8.2 Storage 第一版职责

Storage 只描述业务状态和动作意图：

~~~text
IDLE
  └─ SAVE_PHOTO
       ↓
PREPARING
  └─ STORAGE_READY
       ↓
WRITING
  └─ PHOTO_SAVED
       ↓
SUCCEEDED
~~~

失败进入 FAILED。

Storage Domain 只判断请求和事件是否属于存储领域，不直接调用文件系统。真实文件系统调用放到 APP_Service。

## 8.3 修改 APP_Business/CMakeLists.txt

src_dirs：

~~~cmake
set(src_dirs
    APP_System/src
    APP_Capture/src
    APP_Storage/src
    APP_Test/src
)
~~~

include_dirs：

~~~cmake
set(include_dirs
    APP_System/include
    APP_Capture/include
    APP_Storage/include
    APP_Test/include
)
~~~

## 8.4 main.c 修改

Storage 的 FSM、Controller、Domain 先不接入 main.c。

三层编译成功后，再建立：

~~~text
C:\99_lvgl\components\APP_Business\APP_Test\include\app_storage_test.h
C:\99_lvgl\components\APP_Business\APP_Test\src\app_storage_test.c
~~~

测试文件编译成功后，在 main.c 增加：

~~~c
#include "app_storage_test.h"
~~~

并在 app_main 中增加：

~~~c
app_storage_test_run();
~~~

## 8.5 阶段五正确现象

- CMake 找到 APP_Storage/src。
- Storage 每个源文件都被编译。
- System、Capture 原有代码不受影响。
- Storage 测试能编译后，才把测试入口放进 main.c。
- 最终 ELF 链接成功。

---

# 9. 阶段六：Capture + Storage 集成测试

两个业务域分别成功后，再验证跨领域协议。

## 9.1 新建文件

~~~text
C:\99_lvgl\components\APP_Business\APP_Test\include\app_capture_storage_test.h
C:\99_lvgl\components\APP_Business\APP_Test\src\app_capture_storage_test.c
~~~

## 9.2 流程

~~~text
Capture Domain
  └─ 产生 PHOTO_CAPTURED
       ↓
Dispatcher
       ↓
Storage Domain
  └─ 产生 SAVE_PHOTO Action
       ↓
假 Storage Runtime
       ↓
产生 PHOTO_SAVED Event
       ↓
Capture Domain
       ↓
Capture 状态变为 SUCCEEDED
~~~

重点验证：

- Dispatcher 能找到正确 Domain。
- Request 和 Event 类型一致。
- Capture 和 Storage 通过公共协议通信。
- 业务代码没有直接互相调用。
- State Store 得到最终状态。

## 9.3 CMakeLists.txt 和 main.c

测试文件位于已经收集的 APP_Test/src，因此不需要修改 CMakeLists.txt。

在测试文件编译成功后，main.c 增加：

~~~c
#include "app_capture_storage_test.h"
~~~

并增加：

~~~c
app_capture_storage_test_run();
~~~

如果测试入口变多，可以后续建立 app_business_test.c 统一调用，但不要在集成测试尚未编译成功前做统一重构。

## 9.4 阶段六正确现象

- 集成测试文件能够编译。
- 没有跨业务域函数的隐式声明。
- 没有 Dispatcher、State Store、Request Gateway 的未定义引用。
- 最终 ELF 链接成功。

---

# 10. 阶段七：迁移 APP_UI

## 10.1 新建结构

~~~text
C:\99_lvgl\components\APP_Business\APP_UI
├── include
│   ├── app_ui_fsm.h
│   ├── app_ui_controller.h
│   └── app_ui_domain.h
└── src
    ├── app_ui_fsm.c
    ├── app_ui_controller.c
    └── app_ui_domain.c
~~~

建立顺序仍然是：

~~~text
.h
对应的 .c
编译
下一个 .h
对应的 .c
编译
~~~

## 10.2 UI 业务职责

UI 业务层只表达：

- 当前页面。
- 页面切换。
- 菜单打开和关闭。
- 图库页面。
- 设置页面。
- 错误提示页面。
- 用户操作意图。

UI 业务层暂时不直接调用 LVGL，不直接创建 lv_obj_t。真实 LVGL 控件操作放到 UI Service 或 Adapter。

## 10.3 CMakeLists.txt 和 main.c 修改

APP_Business/CMakeLists.txt 增加：

~~~cmake
APP_UI/src
~~~

和：

~~~cmake
APP_UI/include
~~~

UI 三层先不接入 main.c。UI 假测试完成后，再加入 app_ui_test.h 和 app_ui_test_run。

## 10.4 阶段七正确现象

- UI 业务代码在没有真实 LVGL 控件的情况下可以编译。
- 不出现显示驱动、触摸驱动或 LVGL 实现的未定义引用。
- 最终 ELF 链接成功。

---

# 11. 阶段八：迁移 APP_Web

## 11.1 新建结构

~~~text
C:\99_lvgl\components\APP_Business\APP_Web
├── include
│   ├── app_web_fsm.h
│   ├── app_web_controller.h
│   └── app_web_domain.h
└── src
    ├── app_web_fsm.c
    ├── app_web_controller.c
    └── app_web_domain.c
~~~

## 11.2 Web 业务职责

Web 业务层负责：

- Web 服务启动和停止请求。
- 网络连接状态。
- HTTP 或 WebSocket 业务状态。
- 请求和响应结果。

暂时不要在业务层直接创建 socket、HTTP Server 或 Wi-Fi 连接。真实网络实现放到 Service。

## 11.3 CMakeLists.txt 和 main.c 修改

增加：

~~~cmake
APP_Web/src
~~~

和：

~~~cmake
APP_Web/include
~~~

requires 仍可只使用 APP_Core。确实使用 ESP-IDF 网络组件时，再按实际依赖加入。

Web 三层先不修改 main.c。Web 假测试完成后，再加入 Web 测试入口。

## 11.4 阶段八正确现象

- Web 业务层在没有真实网络连接时也能编译。
- 不出现 socket、HTTP Server 或 Wi-Fi 具体实现的链接错误。
- 最终 ELF 链接成功。

---

# 12. 阶段九：迁移 APP_OTA

## 12.1 新建结构

~~~text
C:\99_lvgl\components\APP_Business\APP_OTA
├── include
│   ├── app_ota_fsm.h
│   ├── app_ota_controller.h
│   └── app_ota_domain.h
└── src
    ├── app_ota_fsm.c
    ├── app_ota_controller.c
    └── app_ota_domain.c
~~~

## 12.2 OTA 业务职责

OTA 业务层只负责：

- OTA 开始。
- OTA 下载中。
- OTA 校验。
- OTA 完成。
- OTA 失败。
- OTA 取消。

真实下载、分区写入和重启操作放到 Service。

## 12.3 CMakeLists.txt 和 main.c 修改

增加：

~~~cmake
APP_OTA/src
~~~

和：

~~~cmake
APP_OTA/include
~~~

OTA 三层先不修改 main.c。OTA 假测试编译成功后，再增加测试入口。

## 12.4 阶段九正确现象

- OTA 业务代码能够单独编译。
- 没有真实 OTA Service 时不会出现链接错误。
- 最终 ELF 链接成功。

---

# 13. 阶段十：整理 APP_Business/CMakeLists.txt

所有业务目录完成并且都能编译后，将 APP_Business/CMakeLists.txt 整理为：

~~~cmake
set(src_dirs
    APP_System/src
    APP_Capture/src
    APP_Storage/src
    APP_UI/src
    APP_Web/src
    APP_OTA/src
    APP_Test/src
)

set(include_dirs
    APP_System/include
    APP_Capture/include
    APP_Storage/include
    APP_UI/include
    APP_Web/include
    APP_OTA/include
    APP_Test/include
)

set(requires
    APP_Core
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

检查：

- 每个 SRC_DIRS 路径真实存在。
- 每个 INCLUDE_DIRS 路径真实存在。
- 不加入空目录。
- 没有重复源文件。
- main/CMakeLists.txt 仍然只编译 main.c。

## 13.1 阶段十正确现象

执行：

~~~powershell
cd C:\99_lvgl
idf.py reconfigure
idf.py build
~~~

应当：

- CMake 一次收集所有业务目录。
- 没有 Directory specified in EXTRA_COMPONENT_DIRS doesn't exist。
- 没有 Cannot find source file。
- 没有 No SOURCES given to target。
- 最终 ELF 链接成功。

---

# 14. 阶段十一：建立 APP_Service

业务层可以在假 Runtime 下编译后，再建立真实服务层。

## 14.1 目录

~~~text
C:\99_lvgl\components\APP_Service
├── CMakeLists.txt
├── include
└── src
~~~

后续可以继续细分：

~~~text
APP_Service
├── APP_Camera
├── APP_Storage
├── APP_UI
├── APP_Web
├── APP_OTA
└── APP_System
~~~

第一版可以先用一个 APP_Service 组件。

## 14.2 Service 职责

Service 把业务 Action 映射到真实实现：

~~~text
Business Action
      ↓
APP_Service
      ↓
APP_Driver / BSP / Middleware
      ↓
真实硬件或系统资源
~~~

例如：

- Capture Action → 摄像头 Service。
- Save Photo Action → 存储 Service。
- UI Action → LVGL Service。
- Web Action → HTTP Service。
- OTA Action → OTA Service。

Service 不负责业务状态判断，FSM 和 Domain 仍然负责业务决策。

## 14.3 APP_Service/CMakeLists.txt

建议起始内容：

~~~cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

set(requires
    APP_Core
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

## 14.4 依赖修改和 main.c

如果 APP_Business 仍只使用公共协议，继续只依赖 APP_Core。

只有业务 Controller 确实需要调用 Service 接口时，再改为：

~~~cmake
set(requires
    APP_Core
    APP_Service
)
~~~

必须保持：

~~~text
APP_Business → APP_Service
APP_Service 不能反过来依赖 APP_Business
~~~

建立 APP_Service 后，不要立即删除假 Runtime 测试，先保留 System 和 Capture 测试。

## 14.5 阶段十一正确现象

- APP_Service 生成自己的组件库。
- APP_Business 与 APP_Service 没有循环依赖。
- 假 Runtime 测试仍可编译。
- 没有开发板时，不要求摄像头、SD 卡、屏幕或网络真正工作。

---

# 15. 阶段十二：建立 APP_Driver、BSP 和 Middlewares

真实硬件代码不要直接放入业务层。

建议：

~~~text
C:\99_lvgl\components
├── APP_Core
├── APP_Business
├── APP_Service
├── APP_Driver
├── BSP
└── Middlewares
~~~

依赖方向：

~~~text
APP_Business
      ↓
APP_Service
      ↓
APP_Driver / BSP / Middlewares
      ↓
ESP-IDF
~~~

职责：

- APP_Driver：摄像头、LCD、触摸、SD 卡等设备驱动封装。
- BSP：具体开发板的引脚、电源、时钟和板级初始化。
- Middlewares：第三方库、协议栈和通用中间件。
- APP_Service：面向业务的服务接口。

main.c 不应直接初始化摄像头、SD 卡、LVGL、Wi-Fi 或 HTTP Server。

没有开发板时，可以使用假实现或条件编译，但接口必须完整并能够编译。

## 15.1 阶段十二正确现象

- 各组件都能生成组件库。
- Business、Service、Driver、BSP 之间无循环依赖。
- 没有因硬件不存在导致的链接错误。
- 最终 ELF 链接成功。

---

# 16. 阶段十三：建立 APP_App 应用组合层

公共层、业务层和服务层稳定后，再建立应用组合层。

## 16.1 新建结构

~~~text
C:\99_lvgl\components\APP_App
├── CMakeLists.txt
├── include
│   └── app_app.h
└── src
    └── app_app.c
~~~

## 16.2 APP_App 职责

APP_App 只负责组合和初始化，不负责实现具体业务。

它统一创建和连接：

1. State Store。
2. Dispatcher。
3. Runtime。
4. System Controller。
5. Capture Controller。
6. Storage Controller。
7. UI、Web、OTA Controller。
8. 各个 Domain。
9. Domain 注册表。
10. Request Gateway。
11. App Task。

初始化顺序：

~~~text
State Store
  ↓
Dispatcher
  ↓
Runtime
  ↓
各业务 Controller
  ↓
各业务 Domain
  ↓
注册 Domain
  ↓
绑定 State Store
  ↓
绑定 Request Gateway
  ↓
启动 App Task
~~~

## 16.3 APP_App/CMakeLists.txt

建议：

~~~cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

set(requires
    APP_Core
    APP_Business
    APP_Service
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

如果 APP_Service 尚未建立，暂时去掉 APP_Service，等建立后再加回。

## 16.4 main/CMakeLists.txt 修改

APP_App 建立并编译成功后，将 main/CMakeLists.txt 中：

~~~cmake
REQUIRES
    APP_Core
    APP_Business
~~~

逐步改为：

~~~cmake
REQUIRES
    APP_App
~~~

如果 main.c 暂时仍直接使用公共类型，可以暂时保留 APP_Core，但最终建议让 main 只依赖 APP_App。

## 16.5 main.c 最终目标

最终 main.c 尽量收敛为：

~~~c
#include "app_app.h"

void app_main(void)
{
    app_app_init();
}
~~~

当前没有开发板时，不要立即删除 APP_Test。可以在 APP_App 内部保留测试开关，或者暂时继续调用假测试，等真实 Service 集成后再清理。

## 16.6 阶段十三正确现象

- main.c 不再直接创建各业务对象。
- main.c 不再包含大量业务头文件。
- APP_App 能够正确链接 APP_Core、APP_Business、APP_Service。
- 没有循环依赖。
- 最终 ELF 链接成功。

---

# 17. 阶段十四：正式启用 Request Gateway 和 App Task

APP_App 可以完成初始化后，再正式接入请求入口和任务调度。

## 17.1 初始化顺序

~~~text
app_core_state_store_init
        ↓
app_core_dispatcher_init
        ↓
app_core_runtime_init
        ↓
app_system_controller_init
app_capture_controller_init
app_storage_controller_init
...
        ↓
app_system_domain_init
app_capture_domain_init
app_storage_domain_init
...
        ↓
app_core_dispatcher_register_domain
        ↓
app_core_request_gateway_init
        ↓
app_core_task_init / start
~~~

## 17.2 正式请求时序

~~~text
UI、Web、定时器或外部输入
              ↓
      Request Gateway
              ↓
          Dispatcher
              ↓
           Domain
              ↓
         Controller
              ↓
             FSM
              ↓
            Action
              ↓
        Action Engine
              ↓
       Runtime / Service
              ↓
             Event
              ↓
          Dispatcher
              ↓
      Domain 更新 State Store
~~~

## 17.3 main.c 修改

当 APP_App 已经承担初始化和任务启动后，main.c 不再直接调用各个业务测试：

~~~c
#include "app_app.h"

void app_main(void)
{
    app_app_init();
}
~~~

## 17.4 阶段十四正确现象

- main.c 只依赖应用组合层。
- 各业务通过 Request、Event、State、Action 公共协议连接。
- 业务层没有直接调用硬件实现。
- 最终 ELF 链接成功。
- 没有开发板时仍然只要求构建成功。

---

# 18. 阶段十五：清理临时代码和旧代码

只有以下条件全部满足后才清理：

1. APP_Core 编译成功。
2. APP_Business 所有业务模块编译成功。
3. APP_Test 所有假测试编译成功。
4. APP_Service 编译成功。
5. APP_App 编译成功。
6. main.c 已经只保留最终入口。
7. 连续多次执行 idf.py build 都成功。

不确定的旧文件不要直接删除，先移动到：

~~~text
C:\99_lvgl\legacy_original
~~~

legacy_original 必须：

- 不加入任何 CMakeLists.txt。
- 不被 SRC_DIRS 收集。
- 不被 main 包含。
- 不被任何组件依赖。

---

# 19. 通用成功判断和故障排查

## 19.1 正常编译成功现象

执行：

~~~powershell
cd C:\99_lvgl
idf.py reconfigure
idf.py build
~~~

正常时应看到：

- Configuring done。
- Generating done。
- 新增 .c 文件进入编译列表。
- Linking CXX executable 成功。
- 最后生成 ELF 和 BIN。
- ninja stopped 不出现 subcommand failed。

## 19.2 Cannot find source file

检查：

1. 文件是否真实存在。
2. CMake 路径是否相对于 APP_Business。
3. 目录名和文件名是否一致。
4. 是否把文件放在 include，却把错误目录写进 src_dirs。
5. CMake 修改后是否执行 idf.py reconfigure。

## 19.3 No SOURCES given to target

通常表示：

- 源目录不存在。
- 源目录为空。
- 路径写错。
- CMake 修改后没有重新配置。

## 19.4 implicit declaration

通常表示：

- 调用文件没有 include 对应头文件。
- 函数名拼写错误。
- 函数声明和定义不一致。
- 接口尚未建立就提前调用。

## 19.5 undefined reference

通常表示：

- 头文件有声明，源文件未加入 CMake。
- 函数只有声明，没有实现。
- 实现被条件编译排除。
- 函数名称或前缀拼写不一致。
- 源文件加入了错误的组件。

## 19.6 路径要求

工程继续放在：

~~~text
C:\99_lvgl
~~~

不要移动回含中文的目录。ESP-IDF 的 CMake、Kconfig 和 Python 工具链可能无法正确处理中文路径。

---

# 20. 当前推荐执行顺序

严格按以下顺序推进：

~~~text
1. APP_Capture/app_capture_fsm.h
2. APP_Capture/app_capture_fsm.c
3. 编译
4. APP_Capture/app_capture_controller.h
5. APP_Capture/app_capture_controller.c
6. 编译
7. APP_Capture/app_capture_domain.h
8. APP_Capture/app_capture_domain.c
9. 编译
10. APP_Test/app_capture_test.h
11. APP_Test/app_capture_test.c
12. 修改 main.c，增加 Capture 测试入口
13. 编译
14. APP_Storage
15. Capture + Storage 集成测试
16. APP_UI
17. APP_Web
18. APP_OTA
19. APP_Service
20. APP_Driver、BSP、Middlewares
21. APP_App
22. 将 main.c 收敛为 app_app_init()
23. 清理临时代码
~~~

当前只做：

~~~text
建立 APP_Capture/app_capture_fsm.h 和 app_capture_fsm.c，
修改 APP_Business/CMakeLists.txt，
重新配置并编译。
~~~

Capture FSM 尚未编译成功前，不要继续建立 Capture Controller、Domain 或测试入口。

---

# 21. 最终目标结构

~~~text
C:\99_lvgl
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── components
    ├── APP_Core
    │   ├── CMakeLists.txt
    │   ├── include
    │   └── src
    ├── APP_Business
    │   ├── CMakeLists.txt
    │   ├── APP_System
    │   ├── APP_Capture
    │   ├── APP_Storage
    │   ├── APP_UI
    │   ├── APP_Web
    │   ├── APP_OTA
    │   └── APP_Test
    ├── APP_Service
    │   ├── CMakeLists.txt
    │   ├── include
    │   └── src
    ├── APP_Driver
    │   ├── CMakeLists.txt
    │   ├── include
    │   └── src
    ├── BSP
    │   ├── CMakeLists.txt
    │   ├── include
    │   └── src
    ├── Middlewares
    │   ├── CMakeLists.txt
    │   ├── include
    │   └── src
    └── APP_App
        ├── CMakeLists.txt
        ├── include
        └── src
~~~

最终依赖方向：

~~~text
main
  ↓
APP_App
  ↓
APP_Business
  ↓
APP_Core

APP_App
  ↓
APP_Service
  ↓
APP_Driver / BSP / Middlewares
  ↓
ESP-IDF
~~~

边界原则：

- APP_Core 不知道 System、Capture、Storage 的具体业务。
- APP_Business 使用公共协议表达业务。
- APP_Service 执行真实动作。
- APP_App 负责组合和初始化。
- main.c 只负责进入应用。


---

# 22. 最底层 Service、Driver、BSP 接口设计

本章专门说明真实业务最终如何连接到摄像头、SD 卡、屏幕、网络和 OTA 等硬件实现。

## 22.1 先区分三种接口

当前工程中有三种不同层次的接口，不能混在一起。

### 第一种：APP_Core Runtime 接口

文件：

~~~text
C:\99_lvgl\components\APP_Core\include\app_core_runtime.h
~~~

其中的回调包括：

~~~text
camera_start_preview
camera_prepare_photo
camera_capture
camera_stop
camera_focus
save_photo
scan_gallery
show_gallery_photo
show_page
web_start
web_stop
ota_begin
ota_finish
ota_abort
system_initialize
system_suspend
system_resume
~~~

这些是“应用层 Runtime 接口”，作用是让 Action Engine 知道如何请求底层服务。

它不是摄像头驱动本身，也不是 SD 卡驱动本身。

### 第二种：APP_Service 接口

Service 负责把 APP_Core 的 Effect 或 Runtime 回调转换成具体服务调用。

例如：

~~~text
app_core_runtime_execute()
        ↓
runtime.camera_capture()
        ↓
app_camera_service_capture()
        ↓
app_camera_driver_capture()
~~~

Service 可以理解为“面向业务的硬件服务适配层”。

### 第三种：APP_Driver 接口

Driver 负责封装具体硬件和 ESP-IDF API。

例如：

~~~text
app_camera_driver_capture()
        ↓
esp_camera_fb_get()
        ↓
真实摄像头
~~~

Driver 不应该知道：

- Capture FSM。
- Capture Domain。
- System 状态。
- APP_CORE_EVENT_TYPE_CAMERA_CAPTURED。
- APP_CAPTURE_STATE_SUCCEEDED。

Driver 只需要返回硬件操作结果、错误码和底层数据。

## 22.2 最终目录结构

建议建立：

~~~text
C:\99_lvgl\components
├── APP_Core
├── APP_Business
├── APP_Service
│   ├── include
│   │   ├── app_camera_service.h
│   │   ├── app_storage_service.h
│   │   ├── app_display_service.h
│   │   ├── app_web_service.h
│   │   └── app_ota_service.h
│   └── src
│       ├── app_camera_service.c
│       ├── app_storage_service.c
│       ├── app_display_service.c
│       ├── app_web_service.c
│       └── app_ota_service.c
├── APP_Driver
│   ├── APP_Camera
│   │   ├── include
│   │   │   └── app_camera_driver.h
│   │   └── src
│   │       └── app_camera_driver.c
│   ├── APP_Storage
│   │   ├── include
│   │   │   └── app_storage_driver.h
│   │   └── src
│   │       └── app_storage_driver.c
│   └── APP_Display
│       ├── include
│       │   └── app_display_driver.h
│       └── src
│           └── app_display_driver.c
├── BSP
└── Middlewares
~~~

如果第一版不想拆太细，也可以先在 APP_Driver/include 和 APP_Driver/src 中统一管理，等驱动数量增加后再拆子目录。

## 22.3 底层接口命名原则

底层接口应按照“硬件能力”命名，而不是按照业务状态命名。

正确：

~~~c
app_camera_driver_capture();
app_storage_driver_write();
app_display_driver_show_page();
app_network_driver_start();
~~~

不建议：

~~~c
app_system_capture_photo();
app_capture_domain_save_photo();
app_ready_state_start_camera();
~~~

原因是 Driver 不应该依赖上层业务名称。

## 22.4 Capture 的完整底层时序

最终拍照流程应该是：

~~~text
Capture FSM
    ↓ 产生
APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE
    ↓
Action Engine
    ↓
app_core_runtime_execute()
    ↓
runtime.camera_capture()
    ↓
APP_Camera Service
    ↓
app_camera_driver_capture()
    ↓
esp_camera / 摄像头硬件
~~~

硬件完成后：

~~~text
摄像头驱动完成
    ↓
APP_Camera Service
    ↓ 组装
APP_CORE_EVENT_TYPE_CAMERA_CAPTURED
    ↓
Dispatcher
    ↓
Capture Domain
    ↓
Capture FSM 更新状态
~~~

Driver 不直接向业务层发送 APP_Core Event。由 Service 将 Driver 的结果转换为公共 Event。

## 22.5 Driver 头文件示例

文件：

~~~text
C:\99_lvgl\components\APP_Driver\APP_Camera\include\app_camera_driver.h
~~~

可以采用以下形式：

~~~c
#ifndef APP_CAMERA_DRIVER_H
#define APP_CAMERA_DRIVER_H

#include "esp_err.h"

#include "app_core_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Camera Driver 控制对象。
     *
     * Driver 只保存硬件驱动上下文，
     * 不保存 Capture FSM 或业务状态。
     */
    typedef struct
    {
        /** 底层摄像头驱动使用的上下文。 */
        void *ctx;

    } app_camera_driver_t;

    /**
     * @brief 初始化摄像头驱动。
     *
     * @param driver 要初始化的 Driver 对象。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_init(
        app_camera_driver_t *driver);

    /**
     * @brief 启动摄像头预览。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_start_preview(
        app_camera_driver_t *driver);

    /**
     * @brief 停止摄像头。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_stop(
        app_camera_driver_t *driver);

    /**
     * @brief 执行一次拍照。
     *
     * @param driver Camera Driver。
     * @param job_id 当前拍照任务编号。
     * @param photo_handle 用于返回照片句柄。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_capture(
        app_camera_driver_t *driver,
        app_core_job_id_t job_id,
        app_core_photo_handle_t *photo_handle);

    /**
     * @brief 执行自动对焦。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_focus(
        app_camera_driver_t *driver);

    /**
     * @brief 释放摄像头驱动。
     *
     * @param driver Camera Driver。
     */
    void app_camera_driver_deinit(
        app_camera_driver_t *driver);

#ifdef __cplusplus
}
#endif

#endif
~~~

对应源文件：

~~~text
C:\99_lvgl\components\APP_Driver\APP_Camera\src\app_camera_driver.c
~~~

这个源文件内部才可以调用：

~~~c
esp_camera_init();
esp_camera_fb_get();
esp_camera_fb_return();
~~~

## 22.6 Service 如何连接 Runtime 和 Driver

Service 的职责是：

1. 接收 Runtime 回调。
2. 调用 Driver。
3. 将 Driver 结果转换为 APP_Core Event。
4. 通过 Event Sink 把 Event 送回 Dispatcher。

示意：

~~~c
esp_err_t app_camera_service_capture(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    app_core_photo_handle_t photo_handle = 0;
    esp_err_t result;

    result = app_camera_driver_capture(
        &service->driver,
        job_id,
        &photo_handle);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_captured_event(
        service,
        meta,
        job_id,
        photo_handle);
}
~~~

然后由 APP_Service 将服务函数绑定到 APP_Core Runtime：

~~~c
runtime->camera_capture =
    app_camera_service_runtime_camera_capture;

runtime->camera_ctx =
    &camera_service;
~~~

这样业务层、Runtime、Service 和 Driver 的关系就是：

~~~text
业务 Action
    ↓
APP_Core Runtime
    ↓
APP_Service
    ↓
APP_Driver
    ↓
硬件
~~~

## 22.7 没有底层接口时应该怎样处理

### 情况一：旧代码已经有硬件实现

例如旧代码中已经有：

~~~c
esp_camera_fb_get();
~~~

不要立即重写。应当：

~~~text
旧摄像头代码
    ↓ 移动并封装
app_camera_driver.c
    ↓ 对外暴露
app_camera_driver.h
    ↓
app_camera_service.c
    ↓
app_core_runtime_t.camera_capture
~~~

这叫“保留原实现并增加适配层”。

### 情况二：旧代码中业务直接调用硬件

例如：

~~~c
void capture_photo(void)
{
    esp_camera_fb_get();
    save_photo_to_sd();
}
~~~

需要拆成：

~~~text
Capture Business
    只产生 Capture Action

Camera Service
    调用 Camera Driver

Storage Service
    调用 Storage Driver
~~~

业务代码中不能出现：

~~~c
esp_camera_fb_get();
fopen();
lv_obj_create();
httpd_start();
~~~

### 情况三：当前没有真实硬件实现

可以先建立假 Driver：

~~~c
esp_err_t app_camera_driver_capture(
    app_camera_driver_t *driver,
    app_core_job_id_t job_id,
    app_core_photo_handle_t *photo_handle)
{
    (void)driver;
    (void)job_id;

    if (photo_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *photo_handle = 1U;

    return ESP_OK;
}
~~~

这样可以先验证：

~~~text
Driver 接口存在
    ↓
Service 能调用 Driver
    ↓
Runtime 能调用 Service
    ↓
Action Engine 能调用 Runtime
    ↓
工程能够编译和链接
~~~

以后拿到开发板，只替换 app_camera_driver.c 内部实现。

## 22.8 哪些底层接口当前已经有 Runtime 入口

当前 APP_Core 已经提供：

| 功能 | Runtime 回调 |
|---|---|
| 摄像头预览 | camera_start_preview |
| 准备拍照 | camera_prepare_photo |
| 拍照 | camera_capture |
| 停止摄像头 | camera_stop |
| 对焦 | camera_focus |
| 保存照片 | save_photo |
| 扫描图库 | scan_gallery |
| 显示图库 | show_gallery_photo |
| 显示页面 | show_page |
| Web 启动 | web_start |
| Web 停止 | web_stop |
| OTA 开始 | ota_begin |
| OTA 完成 | ota_finish |
| OTA 中止 | ota_abort |
| 系统初始化 | system_initialize |
| 系统挂起 | system_suspend |
| 系统恢复 | system_resume |

因此这些功能后续主要补充：

~~~text
APP_Service 实现
APP_Driver 实现
BSP 配置
~~~

通常不需要重新设计 APP_Core。

## 22.9 什么时候需要重新修改 APP_Core

只有公共协议完全缺失时才修改 APP_Core。

例如新增“删除照片”功能，当前协议没有对应定义，就需要同步增加：

~~~text
1. app_core_request.h
   增加 DELETE_PHOTO Request

2. app_core_event.h
   增加 PHOTO_DELETED Event

3. app_core_effect.h
   增加 DELETE_PHOTO Effect

4. app_core_runtime.h
   增加 delete_photo 回调

5. app_core_runtime.c
   增加 Effect 到回调的分发

6. APP_Service
   增加删除照片服务

7. APP_Driver
   增加文件删除驱动

8. APP_Business
   在 Domain、Controller、FSM 中使用

9. APP_Test
   增加假实现测试
~~~

如果现有公共协议已经包含所需动作，就不要重新创造另一套接口。

## 22.10 当前 main.c 中的底层测试代码

当前：

~~~text
C:\99_lvgl\main\main.c
~~~

中：

- 第 50 行附近的 test_runtime_show_page 是假 Runtime 回调。
- 第 226 行附近的 static app_core_runtime_t runtime 是测试用 Runtime 对象。
- 第 181 行附近调用 app_system_test_run()。

这些都不是最终驱动接口，只是当前公共层编译验证代码。

后续应当：

~~~text
当前 main.c 假 Runtime
    ↓
移动到 APP_Test 或 Fake Service

真实 Runtime 组装
    ↓
移动到 APP_App

真实硬件调用
    ↓
移动到 APP_Service 和 APP_Driver
~~~

最终 main.c 只保留：

~~~c
#include "app_app.h"

void app_main(void)
{
    app_app_init();
}
~~~

## 22.11 最终底层接口原则

必须坚持以下原则：

1. Driver 不依赖业务状态机。
2. Driver 不直接包含业务 Domain 头文件。
3. Driver 不直接产生业务 Event。
4. Service 负责业务协议和硬件结果之间的转换。
5. Runtime 是 APP_Core 和 Service 之间的稳定接口。
6. APP_App 负责把 Runtime、Service、Business、Dispatcher 连接起来。
7. 旧硬件代码优先封装，不要一开始全部重写。
8. 当前没有开发板时使用假 Driver，编译成功作为阶段判断标准。
9. 以后更换摄像头或存储芯片时，优先只修改 Driver，不修改 Business。
10. 以后更换业务流程时，优先修改 Business，不修改 Driver。

---

# 附录 A：最近三轮相关会话记录

以下记录按最近三轮已经完成的相关会话整理。

## 第一轮：要求建立后续重构 SOP

### 用户要求

以当前 C:\99_lvgl 工程为准，把后续重构步骤写入 Markdown 文件，要求每个步骤说明：

- 文件架构。
- CMakeLists.txt 修改方式。
- main.c 修改方式。
- 每次重构完成后的正确现象。

### 助手处理结果

创建：

~~~text
C:\99_lvgl\refactor_sop.md
~~~

文档内容包括：

- 当前工程目录基线。
- APP_Core 和 APP_Business 的职责。
- Capture、Storage、UI、Web、OTA 的迁移顺序。
- APP_Service、APP_Driver、BSP、Middlewares 和 APP_App 的后续规划。
- 每个阶段的文件架构。
- APP_Business/CMakeLists.txt 的修改示例。
- main.c 的修改时机。
- CMake、编译和链接错误的判断标准。
- 当前下一步是先迁移 APP_Capture 的 FSM。

当时没有修改现有源码和 CMake 文件。

## 第二轮：询问最终接口如何连接

### 用户问题

最后的接口应该怎么样做？没有接口的重新接吗？

### 助手结论

最终不应该让各业务模块彼此直接调用，而应该通过 APP_Core 的公共协议连接：

~~~text
Request Gateway
    ↓
Dispatcher
    ↓
Domain
    ↓
Controller
    ↓
FSM
    ↓
Action
    ↓
Action Engine
    ↓
Runtime
    ↓
Service
    ↓
Driver
~~~

同时说明：

- 已存在公共 Runtime 接口的功能可以直接复用。
- 没有业务接口的模块需要建立自己的 FSM、Controller、Domain。
- 如果公共协议缺少某个功能，需要同步增加 Request、Event、Effect 和 Runtime。
- 最终可以增加 APP_App 作为统一组合层。
- main.c 最终只保留 app_app_init()。

## 第三轮：明确询问最底层驱动接口

### 用户澄清

所说的接口是最底层的接口，也就是最终业务实现的驱动层。

### 助手结论

底层应分成：

~~~text
APP_Business
    ↓
APP_Core Runtime
    ↓
APP_Service
    ↓
APP_Driver
    ↓
BSP / ESP-IDF
    ↓
真实硬件
~~~

重点说明：

- app_core_runtime.h 是应用层 Runtime 接口，不是硬件驱动本身。
- APP_Service 负责把 Runtime 调用转换成服务调用。
- APP_Driver 负责封装摄像头、存储、屏幕、网络和 OTA 的实际硬件操作。
- Driver 不应该知道 FSM、Domain 和业务状态。
- 旧代码中已有的硬件实现应该先封装，不需要立即重写。
- 当前没有开发板时，可以建立假 Driver，返回 ESP_OK 或 ESP_ERR_NOT_SUPPORTED。
- 如果完全没有某项接口，则建立对应的 app_xxx_driver.h 和 app_xxx_driver.c。
- APP_App 最终负责连接 Runtime、Service、Business、Dispatcher 和 Task。

本次已将以上底层接口设计和最近三轮会话记录补充到本文档中。


---

# 23. 底层接口可直接参照的重构代码

本章补充底层 Driver、Service、Runtime 适配和 APP_App 组合层的代码。

这些代码写入 SOP 供后续手动创建文件时参照。当前不要一次性全部复制到工程中，仍然按照：

~~~text
创建一组 .h/.c
    ↓
修改对应 CMakeLists.txt
    ↓
idf.py reconfigure
    ↓
idf.py build
~~~

的方式推进。

当前建议先完成 Camera 的假 Driver 和 Service。假 Driver 编译通过后，后续再替换为真实摄像头实现。

## 23.1 APP_Driver 目录

~~~text
C:\99_lvgl\components\APP_Driver
├── CMakeLists.txt
└── APP_Camera
    ├── include
    │   └── app_camera_driver.h
    └── src
        └── app_camera_driver.c
~~~

## 23.2 APP_Driver/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\components\APP_Driver\CMakeLists.txt
~~~

代码：

~~~cmake
set(src_dirs
    APP_Camera/src
)

set(include_dirs
    APP_Camera/include
)

set(requires
    esp_common
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

以后真正使用摄像头组件时，再根据实际依赖增加对应组件，例如 esp32-camera。当前假 Driver 不调用具体硬件 API，因此不需要提前增加摄像头依赖。

## 23.3 app_camera_driver.h

文件：

~~~text
C:\99_lvgl\components\APP_Driver\APP_Camera\include\app_camera_driver.h
~~~

完整代码：

~~~c
#ifndef APP_CAMERA_DRIVER_H
#define APP_CAMERA_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Camera Driver 控制对象。
     *
     * 该对象只保存硬件驱动上下文，
     * 不保存 Capture FSM、Domain 或业务状态。
     */
    typedef struct
    {
        /** Driver 是否已经初始化。 */
        bool initialized;

        /** 真实摄像头硬件上下文。 */
        void *hardware_ctx;

    } app_camera_driver_t;

    /**
     * @brief 初始化摄像头驱动。
     *
     * 第一版只完成假初始化。
     * 后续在实现文件中加入 esp_camera_init。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_init(
        app_camera_driver_t *driver);

    /**
     * @brief 启动摄像头预览。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_start_preview(
        app_camera_driver_t *driver);

    /**
     * @brief 准备一次拍照。
     *
     * @param driver Camera Driver。
     * @param job_id 拍照任务编号。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_prepare_photo(
        app_camera_driver_t *driver,
        uint32_t job_id);

    /**
     * @brief 执行一次拍照。
     *
     * Driver 不直接创建 APP_Core Event，
     * 只通过 photo_handle 返回照片资源句柄。
     *
     * @param driver Camera Driver。
     * @param job_id 拍照任务编号。
     * @param photo_handle 照片句柄输出地址。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_capture(
        app_camera_driver_t *driver,
        uint32_t job_id,
        uint32_t *photo_handle);

    /**
     * @brief 执行自动对焦。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_focus(
        app_camera_driver_t *driver);

    /**
     * @brief 停止摄像头。
     *
     * @param driver Camera Driver。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_driver_stop(
        app_camera_driver_t *driver);

    /**
     * @brief 释放摄像头驱动。
     *
     * @param driver Camera Driver。
     */
    void app_camera_driver_deinit(
        app_camera_driver_t *driver);

#ifdef __cplusplus
}
#endif

#endif
~~~

这里故意使用 uint32_t，而不是 app_core_job_id_t 或 app_core_photo_handle_t。

这样做可以让最底层 Driver 不依赖 APP_Core 的业务协议。Service 再负责把底层数据转换成 APP_Core 类型。

## 23.4 app_camera_driver.c

文件：

~~~text
C:\99_lvgl\components\APP_Driver\APP_Camera\src\app_camera_driver.c
~~~

第一版假 Driver 完整代码：

~~~c
#include "app_camera_driver.h"

#include "esp_log.h"

static const char *TAG = "APP_CAMERA_DRIVER";

/**
 * @brief 检查 Camera Driver 是否已经准备好。
 */
static bool app_camera_driver_is_ready(
    const app_camera_driver_t *driver)
{
    return driver != NULL &&
           driver->initialized;
}

esp_err_t app_camera_driver_init(
    app_camera_driver_t *driver)
{
    if (driver == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    driver->initialized = true;
    driver->hardware_ctx = NULL;

    ESP_LOGI(TAG, "fake camera driver initialized");

    return ESP_OK;
}

esp_err_t app_camera_driver_start_preview(
    app_camera_driver_t *driver)
{
    if (!app_camera_driver_is_ready(driver))
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * 后续在这里加入真实摄像头预览初始化。
     * 当前没有开发板，因此只返回 ESP_OK。
     */
    ESP_LOGI(TAG, "fake camera preview started");

    return ESP_OK;
}

esp_err_t app_camera_driver_prepare_photo(
    app_camera_driver_t *driver,
    uint32_t job_id)
{
    if (!app_camera_driver_is_ready(driver) ||
        job_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * 后续在这里设置曝光、白平衡和拍照参数。
     */
    ESP_LOGI(
        TAG,
        "fake camera photo prepared, job_id=%u",
        (unsigned)job_id);

    return ESP_OK;
}

esp_err_t app_camera_driver_capture(
    app_camera_driver_t *driver,
    uint32_t job_id,
    uint32_t *photo_handle)
{
    if (!app_camera_driver_is_ready(driver) ||
        job_id == 0U ||
        photo_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * 当前使用 job_id 生成假的照片句柄。
     * 真实实现中应在这里获取摄像头帧缓冲，
     * 再保存或转换成实际照片资源句柄。
     */
    *photo_handle = job_id;

    ESP_LOGI(
        TAG,
        "fake camera captured, job_id=%u, photo_handle=%u",
        (unsigned)job_id,
        (unsigned)*photo_handle);

    return ESP_OK;
}

esp_err_t app_camera_driver_focus(
    app_camera_driver_t *driver)
{
    if (!app_camera_driver_is_ready(driver))
    {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "fake camera focused");

    return ESP_OK;
}

esp_err_t app_camera_driver_stop(
    app_camera_driver_t *driver)
{
    if (!app_camera_driver_is_ready(driver))
    {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "fake camera stopped");

    return ESP_OK;
}

void app_camera_driver_deinit(
    app_camera_driver_t *driver)
{
    if (driver == NULL)
    {
        return;
    }

    driver->hardware_ctx = NULL;
    driver->initialized = false;
}
~~~

以后拿到开发板时，主要替换这个文件中的函数体，在其中加入：

~~~c
esp_camera_init();
esp_camera_fb_get();
esp_camera_fb_return();
~~~

上层 Driver 头文件、Service、Business 和 APP_Core 不需要因为更换摄像头型号而改变。

---

## 23.5 APP_Service 目录

~~~text
C:\99_lvgl\components\APP_Service
├── CMakeLists.txt
├── include
│   ├── app_camera_service.h
│   └── app_service_runtime.h
└── src
    ├── app_camera_service.c
    └── app_service_runtime.c
~~~

## 23.6 APP_Service/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\components\APP_Service\CMakeLists.txt
~~~

代码：

~~~cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

set(requires
    APP_Core
    APP_Driver
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

依赖方向必须保持：

~~~text
APP_Service → APP_Driver
APP_Driver 不允许反过来依赖 APP_Service
~~~

## 23.7 app_camera_service.h

文件：

~~~text
C:\99_lvgl\components\APP_Service\include\app_camera_service.h
~~~

完整代码：

~~~c
#ifndef APP_CAMERA_SERVICE_H
#define APP_CAMERA_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"

#include "app_core_event.h"
#include "app_core_runtime.h"

#include "app_camera_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Camera Service Event 输出回调。
     *
     * Service 使用该回调向 Dispatcher 发送
     * Camera 相关 Event。
     */
    typedef esp_err_t (*app_camera_service_emit_event_fn)(
        void *ctx,
        const app_core_event_t *event);

    /**
     * @brief Camera Service 控制对象。
     *
     * Service 负责连接 APP_Core Runtime 和 Camera Driver。
     */
    typedef struct
    {
        /** 底层 Camera Driver。 */
        app_camera_driver_t driver;

        /** Event 输出回调。 */
        app_camera_service_emit_event_fn emit_event;

        /** Event 输出回调上下文。 */
        void *event_ctx;

        /** Service 是否已经初始化。 */
        bool initialized;

    } app_camera_service_t;

    /**
     * @brief 初始化 Camera Service。
     *
     * @param service Camera Service。
     * @param emit_event Event 输出回调。
     * @param event_ctx Event 输出上下文。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_service_init(
        app_camera_service_t *service,
        app_camera_service_emit_event_fn emit_event,
        void *event_ctx);

    /**
     * @brief 将 Camera Service 绑定到 APP_Core Runtime。
     *
     * @param service Camera Service。
     * @param runtime APP_Core Runtime。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_camera_service_bind_runtime(
        app_camera_service_t *service,
        app_core_runtime_t *runtime);

    /**
     * @brief 释放 Camera Service。
     *
     * @param service Camera Service。
     */
    void app_camera_service_deinit(
        app_camera_service_t *service);

    /**
     * @brief Service 层启动预览。
     */
    esp_err_t app_camera_service_start_preview(
        app_camera_service_t *service,
        const app_core_message_meta_t *meta);

    /**
     * @brief Service 层准备拍照。
     */
    esp_err_t app_camera_service_prepare_photo(
        app_camera_service_t *service,
        const app_core_message_meta_t *meta,
        app_core_job_id_t job_id);

    /**
     * @brief Service 层执行拍照。
     */
    esp_err_t app_camera_service_capture(
        app_camera_service_t *service,
        const app_core_message_meta_t *meta,
        app_core_job_id_t job_id);

    /**
     * @brief Service 层自动对焦。
     */
    esp_err_t app_camera_service_focus(
        app_camera_service_t *service,
        const app_core_message_meta_t *meta);

    /**
     * @brief Service 层停止摄像头。
     */
    esp_err_t app_camera_service_stop(
        app_camera_service_t *service,
        const app_core_message_meta_t *meta);

#ifdef __cplusplus
}
#endif

#endif
~~~

## 23.8 app_camera_service.c

文件：

~~~text
C:\99_lvgl\components\APP_Service\src\app_camera_service.c
~~~

完整代码：

~~~c
#include "app_camera_service.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "APP_CAMERA_SERVICE";

/*
 * Runtime 到 Service 的适配函数。
 * 这些函数只负责转发参数，不负责业务判断。
 */
static esp_err_t app_camera_service_runtime_start_preview(
    void *ctx,
    const app_core_message_meta_t *meta);

static esp_err_t app_camera_service_runtime_prepare_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id);

static esp_err_t app_camera_service_runtime_capture(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id);

static esp_err_t app_camera_service_runtime_focus(
    void *ctx,
    const app_core_message_meta_t *meta);

static esp_err_t app_camera_service_runtime_stop(
    void *ctx,
    const app_core_message_meta_t *meta);

static bool app_camera_service_is_ready(
    const app_camera_service_t *service)
{
    return service != NULL &&
           service->initialized;
}

/**
 * @brief 发送不携带额外数据的 Camera Event。
 */
static esp_err_t app_camera_service_emit_basic_event(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_event_type_t type)
{
    app_core_event_t event;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL ||
        service->emit_event == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    app_core_event_init(
        &event,
        type,
        meta->request_id,
        meta->parent_request_id,
        APP_CORE_SOURCE_SERVICE,
        APP_CORE_SCOPE_CAPTURE);

    return service->emit_event(
        service->event_ctx,
        &event);
}

/**
 * @brief 发送携带任务编号的 Event。
 */
static esp_err_t app_camera_service_emit_job_event(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_event_type_t type,
    app_core_job_id_t job_id)
{
    app_core_event_t event;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL ||
        job_id == APP_CORE_INVALID_JOB_ID ||
        service->emit_event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_event_init(
        &event,
        type,
        meta->request_id,
        meta->parent_request_id,
        APP_CORE_SOURCE_SERVICE,
        APP_CORE_SCOPE_CAPTURE);

    event.data.job_id = job_id;

    return service->emit_event(
        service->event_ctx,
        &event);
}

/**
 * @brief 发送拍照完成 Event。
 *
 * 当前 Event 数据是 union，
 * 因此 CAMERA_CAPTURED 使用 photo_handle 成员。
 * 请求编号仍然通过 event.meta.request_id 跟踪。
 */
static esp_err_t app_camera_service_emit_captured_event(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_photo_handle_t photo_handle)
{
    app_core_event_t event;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL ||
        photo_handle == APP_CORE_INVALID_PHOTO_HANDLE ||
        service->emit_event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_event_init(
        &event,
        APP_CORE_EVENT_TYPE_CAMERA_CAPTURED,
        meta->request_id,
        meta->parent_request_id,
        APP_CORE_SOURCE_SERVICE,
        APP_CORE_SCOPE_CAPTURE);

    event.data.photo_handle = photo_handle;

    return service->emit_event(
        service->event_ctx,
        &event);
}

esp_err_t app_camera_service_init(
    app_camera_service_t *service,
    app_camera_service_emit_event_fn emit_event,
    void *event_ctx)
{
    esp_err_t result;

    if (service == NULL ||
        emit_event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(
        service,
        0,
        sizeof(*service));

    result = app_camera_driver_init(
        &service->driver);

    if (result != ESP_OK)
    {
        return result;
    }

    service->emit_event = emit_event;
    service->event_ctx = event_ctx;
    service->initialized = true;

    ESP_LOGI(TAG, "camera service initialized");

    return ESP_OK;
}

esp_err_t app_camera_service_bind_runtime(
    app_camera_service_t *service,
    app_core_runtime_t *runtime)
{
    if (!app_camera_service_is_ready(service) ||
        runtime == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    runtime->camera_ctx = service;
    runtime->camera_start_preview =
        app_camera_service_runtime_start_preview;
    runtime->camera_prepare_photo =
        app_camera_service_runtime_prepare_photo;
    runtime->camera_capture =
        app_camera_service_runtime_capture;
    runtime->camera_focus =
        app_camera_service_runtime_focus;
    runtime->camera_stop =
        app_camera_service_runtime_stop;

    return ESP_OK;
}

esp_err_t app_camera_service_start_preview(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta)
{
    esp_err_t result;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_camera_driver_start_preview(
        &service->driver);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_basic_event(
        service,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED);
}

esp_err_t app_camera_service_prepare_photo(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    esp_err_t result;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL ||
        job_id == APP_CORE_INVALID_JOB_ID)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_camera_driver_prepare_photo(
        &service->driver,
        job_id);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_job_event(
        service,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_PREPARED,
        job_id);
}

esp_err_t app_camera_service_capture(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    uint32_t driver_photo_handle = 0U;
    esp_err_t result;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL ||
        job_id == APP_CORE_INVALID_JOB_ID)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_camera_driver_capture(
        &service->driver,
        job_id,
        &driver_photo_handle);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_captured_event(
        service,
        meta,
        (app_core_photo_handle_t)driver_photo_handle);
}

esp_err_t app_camera_service_focus(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta)
{
    esp_err_t result;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_camera_driver_focus(
        &service->driver);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_basic_event(
        service,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_FOCUSED);
}

esp_err_t app_camera_service_stop(
    app_camera_service_t *service,
    const app_core_message_meta_t *meta)
{
    esp_err_t result;

    if (!app_camera_service_is_ready(service) ||
        meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_camera_driver_stop(
        &service->driver);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_camera_service_emit_basic_event(
        service,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_STOPPED);
}

static esp_err_t app_camera_service_runtime_start_preview(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_camera_service_start_preview(
        (app_camera_service_t *)ctx,
        meta);
}

static esp_err_t app_camera_service_runtime_prepare_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    return app_camera_service_prepare_photo(
        (app_camera_service_t *)ctx,
        meta,
        job_id);
}

static esp_err_t app_camera_service_runtime_capture(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    return app_camera_service_capture(
        (app_camera_service_t *)ctx,
        meta,
        job_id);
}

static esp_err_t app_camera_service_runtime_focus(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_camera_service_focus(
        (app_camera_service_t *)ctx,
        meta);
}

static esp_err_t app_camera_service_runtime_stop(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_camera_service_stop(
        (app_camera_service_t *)ctx,
        meta);
}

void app_camera_service_deinit(
    app_camera_service_t *service)
{
    if (service == NULL)
    {
        return;
    }

    app_camera_driver_deinit(
        &service->driver);

    service->emit_event = NULL;
    service->event_ctx = NULL;
    service->initialized = false;
}
~~~

重要说明：

当前 app_core_event_data_t 是 union，因此一个 Event 不能同时保存 job_id 和 photo_handle。

当前示例中：

- CAMERA_PREPARED 使用 data.job_id。
- CAMERA_CAPTURED 使用 data.photo_handle。
- 请求对应关系使用 meta.request_id。

如果以后必须在 CAMERA_CAPTURED 中同时携带 job_id 和 photo_handle，应修改公共 Event 数据结构，新增一个同时包含两个字段的结构体成员，不能继续重复覆盖 union。

---

## 23.9 app_service_runtime.h

文件：

~~~text
C:\99_lvgl\components\APP_Service\include\app_service_runtime.h
~~~

完整代码：

~~~c
#ifndef APP_SERVICE_RUNTIME_H
#define APP_SERVICE_RUNTIME_H

#include <stdbool.h>

#include "esp_err.h"

#include "app_core_runtime.h"

#include "app_camera_service.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief APP_Service Runtime 组合对象。
     *
     * 该对象负责保存各个 Service，
     * 并将它们组合成一个 app_core_runtime_t。
     */
    typedef struct
    {
        /** Camera Service。 */
        app_camera_service_t camera_service;

        /** 组合后的 APP_Core Runtime。 */
        app_core_runtime_t runtime;

        /** 组合对象是否已经初始化。 */
        bool initialized;

    } app_service_runtime_t;

    /**
     * @brief 初始化 APP_Service Runtime。
     *
     * 当前版本只连接 Camera Service。
     * 后续在这里连接 Storage、Display、Web、OTA 和 System Service。
     *
     * @param service_runtime APP_Service Runtime 对象。
     * @param emit_event Event 输出回调。
     * @param event_ctx Event 输出上下文。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_service_runtime_init(
        app_service_runtime_t *service_runtime,
        app_camera_service_emit_event_fn emit_event,
        void *event_ctx);

    /**
     * @brief 获取组合后的 APP_Core Runtime。
     *
     * @param service_runtime APP_Service Runtime 对象。
     * @return 有效 Runtime 指针；无效时返回 NULL。
     */
    const app_core_runtime_t *
    app_service_runtime_get_runtime(
        const app_service_runtime_t *service_runtime);

    /**
     * @brief 释放 APP_Service Runtime。
     *
     * @param service_runtime APP_Service Runtime 对象。
     */
    void app_service_runtime_deinit(
        app_service_runtime_t *service_runtime);

#ifdef __cplusplus
}
#endif

#endif
~~~

## 23.10 app_service_runtime.c

文件：

~~~text
C:\99_lvgl\components\APP_Service\src\app_service_runtime.c
~~~

完整代码：

~~~c
#include "app_service_runtime.h"

#include <string.h>

esp_err_t app_service_runtime_init(
    app_service_runtime_t *service_runtime,
    app_camera_service_emit_event_fn emit_event,
    void *event_ctx)
{
    esp_err_t result;

    if (service_runtime == NULL ||
        emit_event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(
        service_runtime,
        0,
        sizeof(*service_runtime));

    result = app_camera_service_init(
        &service_runtime->camera_service,
        emit_event,
        event_ctx);

    if (result != ESP_OK)
    {
        return result;
    }

    result = app_camera_service_bind_runtime(
        &service_runtime->camera_service,
        &service_runtime->runtime);

    if (result != ESP_OK)
    {
        app_camera_service_deinit(
            &service_runtime->camera_service);

        return result;
    }

    service_runtime->initialized = true;

    return ESP_OK;
}

const app_core_runtime_t *
app_service_runtime_get_runtime(
    const app_service_runtime_t *service_runtime)
{
    if (service_runtime == NULL ||
        !service_runtime->initialized)
    {
        return NULL;
    }

    return &service_runtime->runtime;
}

void app_service_runtime_deinit(
    app_service_runtime_t *service_runtime)
{
    if (service_runtime == NULL)
    {
        return;
    }

    app_camera_service_deinit(
        &service_runtime->camera_service);

    memset(
        &service_runtime->runtime,
        0,
        sizeof(service_runtime->runtime));

    service_runtime->initialized = false;
}
~~~

以后新增 Storage Service 时，在这个结构体中增加 storage_service，并在 init 和 deinit 中完成对应的初始化和释放。

---

# 24. APP_App 最终组合代码

APP_App 是最终连接层，不是新的业务模块。

## 24.1 APP_App/CMakeLists.txt

文件：

~~~text
C:\99_lvgl\components\APP_App\CMakeLists.txt
~~~

代码：

~~~cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

set(requires
    APP_Core
    APP_Business
    APP_Service
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        ${requires}
)
~~~

## 24.2 app_app.h

文件：

~~~text
C:\99_lvgl\components\APP_App\include\app_app.h
~~~

完整代码：

~~~c
#ifndef APP_APP_H
#define APP_APP_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 初始化整个应用。
     *
     * 负责初始化公共层、Service、业务 Controller、
     * 业务 Domain、Request Gateway 和 App Task。
     *
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_app_init(void);

    /**
     * @brief 释放整个应用。
     *
     * @return ESP_OK 表示释放成功。
     */
    esp_err_t app_app_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
~~~

## 24.3 app_app.c 组合顺序示例

文件：

~~~text
C:\99_lvgl\components\APP_App\src\app_app.c
~~~

第一版可以使用以下结构：

~~~c
#include "app_app.h"

#include <stdbool.h>

#include "esp_log.h"

#include "app_core_dispatcher.h"
#include "app_core_request_gateway.h"
#include "app_core_state_store.h"
#include "app_core_task.h"

#include "app_service_runtime.h"

#include "app_system_controller.h"
#include "app_system_domain.h"

static const char *TAG = "APP_APP";

static app_core_state_store_t s_state_store;
static app_core_dispatcher_t s_dispatcher;
static app_service_runtime_t s_service_runtime;
static app_system_controller_t s_system_controller;
static app_system_domain_t s_system_domain;
static app_core_task_t s_task;
static bool s_initialized;

/**
 * @brief Request Gateway 的 Request 提交适配函数。
 */
static esp_err_t app_app_submit_request(
    void *ctx,
    const app_core_request_t *request)
{
    return app_core_dispatcher_submit_request(
        (app_core_dispatcher_t *)ctx,
        request);
}

/**
 * @brief Request Gateway 的状态读取适配函数。
 */
static esp_err_t app_app_read_state(
    void *ctx,
    app_core_state_snapshot_t *snapshot)
{
    return app_core_dispatcher_read_state(
        (const app_core_dispatcher_t *)ctx,
        snapshot);
}

esp_err_t app_app_init(void)
{
    const app_core_runtime_t *runtime;
    esp_err_t result;

    if (s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    result = app_core_state_store_init(
        &s_state_store);

    if (result != ESP_OK)
    {
        return result;
    }

    result = app_core_dispatcher_init(
        &s_dispatcher,
        8U,
        8U);

    if (result != ESP_OK)
    {
        app_core_state_store_deinit(
            &s_state_store);

        return result;
    }

    result = app_core_dispatcher_bind_state_store(
        &s_dispatcher,
        &s_state_store);

    if (result != ESP_OK)
    {
        app_core_dispatcher_deinit(
            &s_dispatcher);

        app_core_state_store_deinit(
            &s_state_store);

        return result;
    }

    result = app_service_runtime_init(
        &s_service_runtime,
        app_core_dispatcher_emit_event,
        &s_dispatcher);

    if (result != ESP_OK)
    {
        app_core_dispatcher_deinit(
            &s_dispatcher);

        app_core_state_store_deinit(
            &s_state_store);

        return result;
    }

    runtime =
        app_service_runtime_get_runtime(
            &s_service_runtime);

    if (runtime == NULL)
    {
        result = ESP_ERR_INVALID_STATE;
        goto cleanup_runtime;
    }

    result = app_system_controller_init(
        &s_system_controller,
        runtime,
        4U,
        app_core_dispatcher_emit_event,
        &s_dispatcher);

    if (result != ESP_OK)
    {
        goto cleanup_runtime;
    }

    result = app_system_domain_init(
        &s_system_domain,
        &s_system_controller,
        &s_state_store);

    if (result != ESP_OK)
    {
        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    result = app_core_dispatcher_register_domain(
        &s_dispatcher,
        app_system_domain_get_handler(
            &s_system_domain));

    if (result != ESP_OK)
    {
        app_system_domain_deinit(
            &s_system_domain);

        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    result = app_core_request_gateway_init();

    if (result != ESP_OK)
    {
        app_system_domain_deinit(
            &s_system_domain);

        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    result = app_core_request_gateway_bind(
        app_app_submit_request,
        &s_dispatcher,
        app_app_read_state,
        &s_dispatcher);

    if (result != ESP_OK)
    {
        app_core_request_gateway_deinit(
            &s_dispatcher);

        app_system_domain_deinit(
            &s_system_domain);

        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    result = app_core_task_init(
        &s_task,
        &s_dispatcher,
        4096U,
        5U,
        10U);

    if (result != ESP_OK)
    {
        app_core_request_gateway_deinit(
            &s_dispatcher);

        app_system_domain_deinit(
            &s_system_domain);

        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    result = app_core_task_start(
        &s_task);

    if (result != ESP_OK)
    {
        app_core_task_deinit(
            &s_task);

        app_core_request_gateway_deinit(
            &s_dispatcher);

        app_system_domain_deinit(
            &s_system_domain);

        app_system_controller_deinit(
            &s_system_controller);

        goto cleanup_runtime;
    }

    s_initialized = true;

    ESP_LOGI(TAG, "application initialized");

    return ESP_OK;

cleanup_runtime:
    app_service_runtime_deinit(
        &s_service_runtime);

    app_core_dispatcher_deinit(
        &s_dispatcher);

    app_core_state_store_deinit(
        &s_state_store);

    return result;
}

esp_err_t app_app_deinit(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    app_core_task_request_stop(
        &s_task);

    app_core_task_deinit(
        &s_task);

    app_core_request_gateway_deinit(
        &s_dispatcher);

    app_system_domain_deinit(
        &s_system_domain);

    app_system_controller_deinit(
        &s_system_controller);

    app_service_runtime_deinit(
        &s_service_runtime);

    app_core_dispatcher_deinit(
        &s_dispatcher);

    app_core_state_store_deinit(
        &s_state_store);

    s_initialized = false;

    return ESP_OK;
}
~~~

这段代码体现的是最终连接顺序：

~~~text
State Store
    ↓
Dispatcher
    ↓
APP_Service Runtime
    ↓
Controller
    ↓
Domain
    ↓
Domain 注册
    ↓
Request Gateway
    ↓
App Task
~~~

后续增加 Capture、Storage、UI、Web 和 OTA 时，只需要在 app_app.c 中增加对应的 Controller、Domain 初始化和注册代码。

---

# 25. main.c 最终代码

当 APP_App 已经建立并成功编译后，文件：

~~~text
C:\99_lvgl\main\main.c
~~~

最终可以收敛为：

~~~c
#include "app_app.h"

#include "esp_log.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    esp_err_t result;

    result = app_app_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "application init failed: %s",
            esp_err_to_name(result));

        return;
    }

    ESP_LOGI(TAG, "application started");
}
~~~

main/CMakeLists.txt 最终修改为：

~~~cmake
idf_component_register(
    SRCS
        "main.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        APP_App
)
~~~

在此之前，main.c 中的公共层测试代码不要立即删除。应当等 APP_App 能够成功编译并连接后，再把以下内容移到 APP_Test：

- test_domain_match_request。
- test_domain_handle_request。
- test_runtime_show_page。
- test_gateway_submit。
- test_gateway_read_state。
- test_action_engine_emit_event。
- 直接创建的 State Store。
- 直接创建的 Dispatcher。
- 直接创建的 Runtime。
- 直接创建的 Action Engine。

---

# 26. 底层代码的迁移顺序

严格按照以下顺序：

~~~text
1. 建立 APP_Driver/CMakeLists.txt
2. 建立 app_camera_driver.h
3. 建立 app_camera_driver.c
4. 修改 APP_Driver/CMakeLists.txt
5. 编译

6. 建立 APP_Service/CMakeLists.txt
7. 建立 app_camera_service.h
8. 建立 app_camera_service.c
9. 建立 app_service_runtime.h
10. 建立 app_service_runtime.c
11. 修改 APP_Service/CMakeLists.txt
12. 编译

13. 将 app_core_runtime_t.camera_capture 等回调指向 Camera Service
14. 编译

15. 建立 APP_Capture Controller 和 Domain
16. 编译

17. 建立 Capture 假测试
18. 编译

19. 建立 APP_App
20. 编译

21. 最后修改 main.c
~~~

每次检查这条调用链：

~~~text
Capture FSM 产生 Effect
    ↓
Action Engine 取出 Action
    ↓
app_core_runtime_execute
    ↓
runtime.camera_capture
    ↓
app_camera_service_runtime_capture
    ↓
app_camera_service_capture
    ↓
app_camera_driver_capture
    ↓
返回照片句柄
    ↓
Service 组装 CAMERA_CAPTURED Event
    ↓
Dispatcher
    ↓
Capture Domain
~~~

如果链条中间没有对应函数，就补对应层接口，不要让上层直接跨层调用。

---

# 27. 其他底层模块的代码扩展方式

## 27.1 Storage

~~~text
app_storage_driver.h/.c
    ↓
app_storage_service.h/.c
    ↓
runtime.save_photo
runtime.scan_gallery
runtime.show_gallery_photo
~~~

## 27.2 Display

~~~text
app_display_driver.h/.c
    ↓
app_display_service.h/.c
    ↓
runtime.show_page
runtime.update_menu_selection
runtime.show_gallery_photo
~~~

## 27.3 Web

~~~text
app_network_driver.h/.c
    ↓
app_web_service.h/.c
    ↓
runtime.web_start
runtime.web_stop
~~~

## 27.4 OTA

~~~text
app_ota_driver.h/.c
    ↓
app_ota_service.h/.c
    ↓
runtime.ota_begin
runtime.ota_finish
runtime.ota_abort
~~~

## 27.5 System

~~~text
app_system_driver.h/.c
    ↓
app_system_service.h/.c
    ↓
runtime.system_initialize
runtime.system_suspend
runtime.system_resume
~~~

它们都遵守同一套规则：

~~~text
Runtime
    ↓
Service
    ↓
Driver
    ↓
BSP / ESP-IDF
~~~

---

# 28. 底层接口缺失时的判断方法

## 28.1 Runtime 已经有对应回调

例如已有 camera_capture：

~~~text
不修改 APP_Core
    ↓
新增 app_camera_service.h/.c
    ↓
新增 app_camera_driver.h/.c
    ↓
绑定 runtime.camera_capture
~~~

## 28.2 Effect 已存在，但 Runtime 没有正确实现

检查：

~~~text
app_core_runtime.h
app_core_runtime.c
APP_Service
APP_Driver
~~~

需要补齐：

1. Runtime 回调字段。
2. Runtime 执行分支。
3. Service 函数。
4. Driver 函数。
5. 测试代码。

## 28.3 Request、Event、Effect 都不存在

说明公共协议确实缺失，需要依次补充：

~~~text
Request
    ↓
Event
    ↓
Effect
    ↓
Runtime
    ↓
Service
    ↓
Driver
    ↓
Test
~~~

## 28.4 只有旧 Driver 实现，没有接口

保留旧 Driver 的实现，补充：

~~~text
Driver .h
    ↓
Service .h
    ↓
Runtime 绑定
~~~

不要一开始全部重写硬件代码。

---

# 29. 完成底层接口后的正确现象

完成 Camera Driver 和 Camera Service 后：

1. APP_Driver 能够生成组件库。
2. APP_Service 能够生成组件库。
3. APP_Service 能够找到 app_camera_driver.h。
4. APP_Core Runtime 类型能够被 APP_Service 正确使用。
5. 假 Driver 能够返回 ESP_OK。
6. 没有 Cannot find source file。
7. 没有 No SOURCES given to target。
8. 没有 implicit declaration。
9. 没有 undefined reference。
10. 最终 ELF 链接成功。

当前不要求：

- 摄像头真的出图。
- SD 卡真的写入。
- 屏幕真的显示。
- 网络真的连接。
- OTA 真的升级。

拿到开发板以后，主要替换 Driver 内部实现，再进行真实硬件验证。

