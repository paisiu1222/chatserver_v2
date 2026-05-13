cmake_minimum_required(VERSION 3.0)
project(chat)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 本地依赖根路径
set(LOCAL_DEPS "/tmp/local_deps/install")
set(MUDUO_INSTALL "/tmp/muduo_install")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

# 第三方库头文件路径
include_directories(${MUDUO_INSTALL}/include)
include_directories(${LOCAL_DEPS}/usr/include)
include_directories(${LOCAL_DEPS}/usr/include/mysql)

# 第三方库链接路径
link_directories(${MUDUO_INSTALL}/lib)
link_directories(${LOCAL_DEPS}/usr/lib/x86_64-linux-gnu)

add_subdirectory(src)