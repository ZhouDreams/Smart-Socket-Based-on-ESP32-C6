# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Smart-Socket-Based-on-ESP32-C6)

# 添加SPIFFS文件系统支持
spiffs_create_partition_image(storage ./spiffs FLASH_IN_PROJECT)