cmake_minimum_required(VERSION 3.14)

project(Sulfur)

find_package(Vulkan REQUIRED)

add_library(sulfur SHARED
  src/instance.c
  src/device.c
  src/swapchain.c
  src/buffer.c
  src/shader.c
  src/pipeline.c
  src/texture.c)

target_include_directories(sulfur PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(sulfur PUBLIC Vulkan::Vulkan)

add_library(Sulfur::Sulfur ALIAS sulfur)
