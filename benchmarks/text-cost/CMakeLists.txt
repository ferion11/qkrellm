# SPDX-License-Identifier: GPL-3.0-or-later
# Standalone experiment for ADR 0011 (not added to the main build). Usage: see main.cpp.
cmake_minimum_required(VERSION 3.25)
project(qkrellm_text_cost LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
add_executable(text-cost main.cpp)
target_link_libraries(text-cost PRIVATE Qt6::Widgets)
