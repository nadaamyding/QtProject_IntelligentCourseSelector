cmake_minimum_required(VERSION 3.5)
project(IntelligentCourseSelector)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic和rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt6组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 设置包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
)

# 收集源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# 收集头文件
file(GLOB_RECURSE HEADERS
    "include/*.h"
)

# 收集UI文件
file(GLOB_RECURSE FORMS
    "ui/*.ui"
)

# 添加可执行文件
add_executable(IntelligentCourseSelector
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# 链接Qt库
target_link_libraries(IntelligentCourseSelector
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 复制数据文件到构建目录
add_custom_command(TARGET IntelligentCourseSelector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/bin/data
    COMMENT "Copying data files to build directory"
)

# 复制nlohmann库到构建目录
add_custom_command(TARGET IntelligentCourseSelector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/thirdparty/nlohmann
        ${CMAKE_BINARY_DIR}/bin/thirdparty/nlohmann
    COMMENT "Copying nlohmann library to build directory"
)

# 添加 UTF-8 编码支持
if (MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# 安装配置
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS IntelligentCourseSelector
    RUNTIME DESTINATION bin
)

install(DIRECTORY data/ DESTINATION data)
install(DIRECTORY thirdparty/ DESTINATION thirdparty)
