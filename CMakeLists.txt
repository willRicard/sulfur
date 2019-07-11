cmake_minimum_required(VERSION 3.10)

project(Sulfur)

find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
  find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
  find_library(VULKAN_LIBRARY vulkan)
  add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
  set_target_properties(Vulkan::Vulkan PROPERTIES
          IMPORTED_LOCATION ${VULKAN_LIBRARY}
          INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_INCLUDE_DIR})
endif(NOT Vulkan_FOUND)

add_library(sulfur SHARED
  src/device.c
  src/swapchain.c
  src/buffer.c
  src/shader.c
  src/pipeline.c
  src/texture.c
  src/debug.c)

target_include_directories(sulfur PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(sulfur PUBLIC Vulkan::Vulkan)

add_library(Sulfur::Sulfur ALIAS sulfur)
