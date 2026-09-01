# 99_lvgl 项目重构会话记录

> 记录时间：2026-09-01
>
> 当前工程：`C:\99_lvgl`
>
> 参考工程：`C:\日志\991_newstructure`

## 一、重构目标

本次工作是在原始 ESP-IDF + LVGL 工程基础上，逐步进行结构化重构。重构过程中遵循以下原则：

1. 先备份原始代码，再建立新的目录结构。
2. 先迁移公共层，确保公共协议和数据结构稳定。
3. `.h` 与 `.c` 文件成对创建和验证。
4. 没有开发板时，以编译和链接成功作为当前阶段的主要验证标准。
5. 硬件驱动、LVGL、Web、OTA 等真实功能暂不在公共层中实现。
6. 工程路径使用纯英文，避免 ESP-IDF Kconfig 在中文路径下出现路径编码问题。

## 二、路径问题及解决结果

工程曾经放在带中文目录的路径下，ESP-IDF 配置阶段出现了类似以下问题：

```text
FileNotFoundError: C:/鏃ュ織/99_lvgl/build/kconfigs.in
```

原因是 ESP-IDF/Kconfig 处理工程路径时发生中文路径编码转换。

最终将工程移动到：

```text
C:\99_lvgl
```

移动后，CMake 配置和 ESP-IDF 编译流程恢复正常。

## 三、当前目录结构

```text
C:\99_lvgl
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── main.c
├── components
│   └── APP_Core
│       ├── CMakeLists.txt
│       ├── include
│       │   ├── app_core_action.h
│       │   ├── app_core_dispatcher.h
│       │   ├── app_core_domain.h
│       │   ├── app_core_effect.h
│       │   ├── app_core_event.h
│       │   ├── app_core_request.h
│       │   ├── app_core_request_gateway.h
│       │   ├── app_core_runtime.h
│       │   ├── app_core_state.h
│       │   ├── app_core_state_store.h
│       │   └── app_core_types.h
│       └── src
│           ├── app_core_action.c
│           ├── app_core_dispatcher.c
│           ├── app_core_domain.c
│           ├── app_core_effect.c
│           ├── app_core_event.c
│           ├── app_core_request.c
│           ├── app_core_request_gateway.c
│           ├── app_core_runtime.c
│           ├── app_core_state.c
│           ├── app_core_state_store.c
│           └── app_core_types.c
├── legacy_original
├── pc_server
├── tools
├── sdkconfig
├── sdkconfig.defaults
├── partitions-16MiB.csv
└── session.md
```

`legacy_original` 用于保存原始代码，不参与当前工程构建。

## 四、APP_Core 当前分层

### 1. `app_core_types`

提供公共基础类型和协议定义，包括：

- Request ID、Job ID、Photo Handle；
- 消息来源 `app_core_source_t`；
- 业务范围 `app_core_scope_t`；
- 系统状态、拍照状态、Web 状态、存储状态、OTA 状态；
- LVGL 页面类型；
- Request、Event、Effect 共用的消息元数据；
- 初始化、合法性检查和字符串转换函数。

### 2. `app_core_request`

表示外部或内部发起的“请求做什么”，例如拍照、启动 Web、进入图库等。

### 3. `app_core_domain`

表示业务域处理器。Domain 负责判断自己是否处理某个 Request 或 Event，并提供对应的处理函数。

### 4. `app_core_effect`

表示业务逻辑决定之后，需要底层 Runtime 执行的具体动作，例如启动预览、保存照片、切换页面等。

### 5. `app_core_action`

表示一次具体动作的执行对象，保存动作编号、Effect 和动作状态。

### 6. `app_core_event`

表示动作执行完成后产生的结果或系统事实，例如 Camera 拍摄完成、照片保存完成等。

### 7. `app_core_state`

定义系统状态快照，描述系统当前的整体状态。

### 8. `app_core_state_store`

负责保存和读取系统状态快照，并使用 FreeRTOS 互斥锁保护并发访问。

### 9. `app_core_dispatcher`

负责接收 Request 和 Event，并将消息分发给已注册的 Domain。

当前已经支持：

- Request 队列；
- Event 队列；
- Domain 注册；
- State Store 绑定；
- 单次处理 Request、Event 和 Domain 周期函数。

### 10. `app_core_request_gateway`

为 LVGL、按键、Web 或其他外部模块提供统一的 Request 提交入口和状态读取入口。

Gateway 通过回调与 Dispatcher、State Store 解耦，并负责为没有 Request ID 的请求自动分配 ID。

### 11. `app_core_runtime`

负责把 Effect 转换为具体的底层回调调用。

Runtime 本身不直接实现 Camera、LVGL、Web 或 OTA，而是保存各个底层模块提供的回调函数。这样 App Core 只依赖抽象接口，不直接依赖具体驱动。

## 五、当前 CMake 配置

### 根目录 CMake

根目录 [CMakeLists.txt](C:/99_lvgl/CMakeLists.txt) 使用 ESP-IDF 标准初始化顺序：

```cmake
cmake_minimum_required(VERSION 3.16)
add_compile_options(-fdiagnostics-color=always)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(25_1_camera)
```

### APP_Core CMake

`APP_Core/CMakeLists.txt` 使用目录方式收集源码：

```cmake
set(src_dirs
    src
)

set(include_dirs
    include
)

idf_component_register(
    SRC_DIRS
        ${src_dirs}

    INCLUDE_DIRS
        ${include_dirs}

    REQUIRES
        freertos
        esp_common
)
```

以后新增 `components/APP_Core/src` 下的 `.c` 文件时，不需要再次修改 CMakeLists，只需要执行重新配置。

### main CMake

`main/CMakeLists.txt` 依赖 `APP_Core`：

```cmake
idf_component_register(
    SRCS
        "main.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        APP_Core
)
```

## 六、当前 `main.c` 验证内容

当前 [main.c](C:/99_lvgl/main/main.c) 只承担公共层冒烟测试，不承担正式业务逻辑。

已经验证：

1. Request 初始化、合法性检查和字符串转换；
2. Event 初始化、合法性检查和字符串转换；
3. Effect 初始化、合法性检查和字符串转换；
4. Action 初始化和合法性检查；
5. State Snapshot 初始化和合法性检查；
6. Domain 初始化、Request 匹配、Event 匹配和周期处理；
7. Dispatcher 初始化、Domain 注册、Request 提交、Event 发送和单次处理；
8. State Store 初始化、状态发布和状态读取；
9. Request Gateway 初始化、绑定、提交 Request、自动分配 Request ID 和读取状态；
10. Runtime 使用假回调执行页面切换 Effect。

由于当前没有开发板，Runtime 的验证使用返回 `ESP_OK` 的假回调，目的在于确认接口能够正确编译、链接和调用。

Dispatcher 当前的独立冒烟测试没有绑定 State Store，这不是当前阶段的错误。后续完成整体运行时装配时，需要将 Dispatcher 与实际 State Store 绑定。

## 七、最终构建检查结果

最终构建目录：

```text
C:\99_lvgl\build
```

已确认生成：

```text
C:\99_lvgl\build\25_1_camera.elf
C:\99_lvgl\build\25_1_camera.bin
C:\99_lvgl\build\25_1_camera.map
```

已确认 `build.ninja` 包含：

```text
app_core_runtime.c.obj
```

并且 `app_core_runtime.c.obj` 已被加入 `libAPP_Core.a`。

当前 `APP_Core/src` 共 11 个 `.c` 文件，均不是空文件。

截至本记录生成时，没有发现编译或链接错误。

## 八、Git 忽略状态

当前 `.gitignore` 已忽略：

- `build/`；
- `.cache/`；
- `legacy_original/`；
- `managed_components/`；
- Python 的 `.venv/` 和 `venv/`；
- Python 缓存目录；
- CMake、Ninja 和编译产物；
- `sdkconfig.old` 等 ESP-IDF 自动生成的旧配置文件。

此前 GitHub 拒绝推送的原因是 `pc_server/.venv` 中的 `_polars_runtime.pyd` 超过 GitHub 100 MB 单文件限制。现在 `.venv` 已加入忽略规则，不应再被新的提交纳入。

`pc_server/yolo11n.pt` 当前仍是 Git 跟踪文件。如果该模型是 PC 端服务运行所必需的，可以保留；如果不希望上传模型文件，需要另外将它加入 `.gitignore`，并使用 `git rm --cached pc_server/yolo11n.pt` 从 Git 索引中移除。

## 九、当前限制

本阶段只能说明：

- CMake 配置成功；
- 所有当前公共层源码成功编译；
- 所有当前公共层对象成功链接；
- 假回调接口能够被调用。

本阶段还不能说明：

- Camera 硬件工作正常；
- LCD、LVGL 页面显示正常；
- SD 卡和照片存储正常；
- Web 服务正常；
- OTA 流程正常；
- FreeRTOS 多任务并发下没有竞态问题。

## 十、建议的下一步

下一步应进行 App Core 的整体运行时装配：

1. 创建统一的 App Core Runtime 实例；
2. 创建实际的 State Store 实例；
3. 初始化 Dispatcher；
4. 将 Dispatcher 绑定到 State Store；
5. 注册 Camera、UI、Storage、Web、OTA 等 Domain；
6. 将 Dispatcher 的 Request 提交函数绑定到 Request Gateway；
7. 将各个底层 Service 或 Driver 的接口绑定到 Runtime 回调；
8. 最后再逐个迁移真实硬件和业务模块。

在开发板可用之前，仍然以“每迁移一个模块，必须能够独立编译和链接”为阶段验收标准。

## 十一、会话过程摘要

1. 讨论从零设计和重构工程的方法；
2. 根据参考工程制定逐文件迁移 SOP；
3. 将原始工程备份到 `legacy_original`，建立新的迁移基线；
4. 修改 CMake，使新目录成为当前构建入口；
5. 先搭建公共类型、Request、Event、Effect、Action、State 等基础模块；
6. 修复 Request 类型接口和实现缺失导致的编译/链接问题；
7. 增加 Domain、Dispatcher、State Store 和 Request Gateway；
8. 检查并完善 `.gitignore`，避免 `build` 和 Python 虚拟环境被提交；
9. 处理中文路径导致的 ESP-IDF Kconfig 错误；
10. 将工程移动到 `C:\99_lvgl`；
11. 搭建 Runtime，并在 `main.c` 中加入假回调验证；
12. 最终确认 Runtime 已进入 CMake 构建和链接流程；
13. 当前公共层构建成功，准备进入整体运行时装配阶段。
