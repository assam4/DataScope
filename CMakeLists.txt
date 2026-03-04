cmake_minimum_required(VERSION 3.23)
project(stats_logger)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# external dependencies
include(FetchContent)
find_package(tomlplusplus QUIET)
if (NOT tomlplusplus_FOUND)
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0  # ← Вместо v1.x
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()
find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.x
    )
    FetchContent_MakeAvailable(spdlog)
endif()
find_package(Boost QUIET)
find_package(Boost REQUIRED COMPONENTS program_options)
if (NOT Boost_FOUND)
    message(WARNING "Boost not found. Please install it before run cmake")
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/toml_config_parser.cpp
    src/logger.cpp
    src/parser_csv.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${tomlplusplus_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    tomlplusplus::tomlplusplus
    spdlog::spdlog
    Boost::program_options
    Boost::boost
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

