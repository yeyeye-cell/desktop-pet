cmake_minimum_required(VERSION 3.16)
project(DesktopPet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Qt 6.11.0 MinGW
set(CMAKE_PREFIX_PATH "D:/qtinstaal/6.11.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# --- Source files ---
set(SOURCES
    src/main.cpp
    src/PetWindow.cpp
    src/PetController.cpp
    src/SpriteManager.cpp
    src/AnimationEngine.cpp
    src/BehaviorEngine.cpp
    src/SettingsDialog.cpp
)

set(HEADERS
    src/SpriteData.h
    src/PetWindow.h
    src/PetController.h
    src/SpriteManager.h
    src/AnimationEngine.h
    src/BehaviorEngine.h
    src/SettingsDialog.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# MinGW: no WIN32_EXECUTABLE so we can see console output during dev
# Uncomment for release:
# set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

# --- Tests (optional, build with -DBUILD_TESTS=ON) ---
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(test_sprite_manager
        tests/test_sprite_manager.cpp
        src/SpriteManager.cpp
        src/SpriteData.h
    )
    target_include_directories(test_sprite_manager PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_sprite_manager PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

    add_executable(test_animation_engine
        tests/test_animation_engine.cpp
        src/AnimationEngine.cpp
        src/SpriteData.h
    )
    target_include_directories(test_animation_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_animation_engine PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

    add_executable(test_behavior_engine
        tests/test_behavior_engine.cpp
        src/BehaviorEngine.cpp
        src/SpriteData.h
    )
    target_include_directories(test_behavior_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_behavior_engine PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
endif()
