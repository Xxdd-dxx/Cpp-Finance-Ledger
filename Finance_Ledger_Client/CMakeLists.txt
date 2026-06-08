cmake_minimum_required(VERSION 3.16)
project(FinanceClient VERSION 1.0 LANGUAGES CXX)

# 启用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 的 MOC, UIC 和 RCC (核心机制)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 寻找 Qt5 核心库和网络模块
find_package(Qt5 COMPONENTS Widgets Network Gui REQUIRED)

# 添加所有的源码和 UI 文件
add_executable(FinanceClient
    main.cpp
    LoginWindow.cpp
    LoginWindow.h
    mainwindow.cpp
    mainwindow.h
    loginwindow.ui
    mainwindow.ui
    registerwindow.h
    registerwindow.cpp
    registerwindow.ui
    res.qrc
    transactiondialog.h transactiondialog.cpp transactiondialog.ui
)

# 链接 Qt5 库
target_link_libraries(FinanceClient PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Gui
)