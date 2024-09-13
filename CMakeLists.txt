cmake_minimum_required(VERSION 3.7.0)
project(VitraeEngine VERSION 0.1.0)

include(CTest)
enable_testing()

set(VITRAE_ENABLE_STRINGID_DEBUGGING OFF CACHE BOOL "Whether StringId objects will keep info on original strings for debugging purposes. Has performance penalty.")
set(VITRAE_ENABLE_MMETER OFF CACHE BOOL "Whether MMeter profiling is enabled. If On, VitraeEngine/dependencies/MMeter/src/MMeter.cpp has to be added to the application's source file list")
set(VITRAE_ENABLE_DETERMINISTIC_RENDERING_TIMES OFF CACHE BOOL "Whether CPU should wait for rendering operations to finish before issuing new commands. This is useful for debugging and profiling. Might have performance impact.")

file(GLOB_RECURSE SrcFiles CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS include/*.h include/*.hpp)
set(GLAD_SrcFile ../VitraeEngine/dependencies/glad/src/glad.c)

add_library(VitraeEngine ${SrcFiles} ${HeaderFiles} ${GLAD_SrcFile})
target_compile_features(VitraeEngine PUBLIC cxx_std_20)
target_include_directories(VitraeEngine PUBLIC include)
target_include_directories(VitraeEngine PUBLIC ../VitraeEngine/dependencies/)
target_include_directories(VitraeEngine PUBLIC ../VitraeEngine/dependencies/glad/include)
target_include_directories(VitraeEngine PUBLIC ../VitraeEngine/dependencies/DynAsMa/include)
target_include_directories(VitraeEngine PUBLIC ../VitraeEngine/dependencies/MMeter/include)
target_link_libraries(VitraeEngine PUBLIC assimp glfw)

if(VITRAE_ENABLE_STRINGID_DEBUGGING)
    target_compile_definitions(VitraeEngine PUBLIC VITRAE_DEBUG_STRINGIDS)
endif()

if(VITRAE_ENABLE_MMETER)
    target_compile_definitions(VitraeEngine PUBLIC MMETER_ENABLE=1)
else()
    target_compile_definitions(VitraeEngine PUBLIC MMETER_ENABLE=0)
endif()

if(VITRAE_ENABLE_DETERMINISTIC_RENDERING_TIMES)
    target_compile_definitions(VitraeEngine PUBLIC VITRAE_ENABLE_DETERMINISTIC_RENDERING)
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
