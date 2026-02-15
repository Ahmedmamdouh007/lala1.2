cmake_minimum_required(VERSION 3.14)
project(lala_store_backend CXX)

set(CMAKE_CXX_STANDARD 17)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

# Security lab module (routes only compile when ENABLE_LABS=ON)
option(ENABLE_LABS "Build security lab module (lab routes)" OFF)

# Fetch Crow
include(FetchContent)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.3.0
)
FetchContent_MakeAvailable(crow)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBPQXX_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    main.cpp
    db/connection.cpp
)
if(ENABLE_LABS)
    list(APPEND SOURCES routes/lab_routes.cpp lab/validation_demo/validation_demo.cpp lab/telemetry/lab_telemetry.cpp lab_services/tcp_lab_server.cpp)
    add_compile_definitions(ENABLE_LABS)
endif()

add_executable(lala_backend ${SOURCES})

target_link_libraries(lala_backend PRIVATE
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBPQXX_LIBRARIES}
)

target_include_directories(lala_backend PRIVATE
    ${LIBPQXX_INCLUDE_DIRS}
)

# -----------------------------------------------------------------------------
# Memory lab targets (standalone binaries for ASan/memory-safety training)
# BUILD_MEMORY_LABS=ON: build all 8. MEMORY_LABS_ASAN=ON: build with AddressSanitizer.
# Profiles: normal debug (default), asan debug (-DMEMORY_LABS_ASAN=ON).
# -----------------------------------------------------------------------------
option(BUILD_MEMORY_LABS "Build memory lab targets (stack/heap overflow, UAF, etc.)" OFF)
option(MEMORY_LABS_ASAN "Build memory lab targets with AddressSanitizer (asan debug profile)" OFF)

if(BUILD_MEMORY_LABS)
    set(MEMORY_LAB_SOURCES
        lab_targets/stack_buffer_overflow.cpp
        lab_targets/heap_buffer_overflow.cpp
        lab_targets/use_after_free.cpp
        lab_targets/double_free.cpp
        lab_targets/out_of_bounds_read.cpp
        lab_targets/null_deref.cpp
        lab_targets/integer_overflow_to_alloc.cpp
        lab_targets/format_string_bug.cpp
        lab_targets/unsafe_strncpy.cpp
        lab_targets/stack_read_overflow.cpp
        lab_targets/fixed_buffer_overflow.cpp
    )
    set(MEMORY_LAB_NAMES
        stack_buffer_overflow
        heap_buffer_overflow
        use_after_free
        double_free
        out_of_bounds_read
        null_deref
        integer_overflow_to_alloc
        format_string_bug
        unsafe_strncpy
        stack_read_overflow
        fixed_buffer_overflow
    )
    set(MEMORY_LAB_DEBUG_FLAGS "-g" "-fno-omit-frame-pointer")
    if(MEMORY_LABS_ASAN)
        list(APPEND MEMORY_LAB_DEBUG_FLAGS "-fsanitize=address")
        set(MEMORY_LAB_LINK_FLAGS "-fsanitize=address")
    endif()
    list(LENGTH MEMORY_LAB_SOURCES N_LABS)
    math(EXPR N_LABS "${N_LABS} - 1")
    foreach(i RANGE ${N_LABS})
        list(GET MEMORY_LAB_SOURCES ${i} SRC)
        list(GET MEMORY_LAB_NAMES ${i} NAME)
        add_executable(${NAME} ${SRC})
        target_compile_options(${NAME} PRIVATE ${MEMORY_LAB_DEBUG_FLAGS})
        if(MEMORY_LAB_LINK_FLAGS)
            target_link_options(${NAME} PRIVATE ${MEMORY_LAB_LINK_FLAGS})
        endif()
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Fuzz targets (standalone binaries, libFuzzer + AddressSanitizer + UBSan)
# Build with: cmake -DBUILD_FUZZ_TARGETS=ON -DCMAKE_CXX_COMPILER=clang++ ..
# Fuzz internal parsing functions (not HTTP). Seed corpus: backend/fuzz_corpus/
# -----------------------------------------------------------------------------
option(BUILD_FUZZ_TARGETS "Build libFuzzer fuzz targets (requires Clang)" OFF)

if(BUILD_FUZZ_TARGETS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(FUZZ_FLAGS "-fsanitize=fuzzer,address,undefined" "-fno-omit-frame-pointer" "-g")
        set(FUZZ_LINK_FLAGS "-fsanitize=fuzzer,address,undefined")

        add_executable(fuzz_json_parser fuzz_targets/fuzz_json_parser.cpp)
        target_compile_definitions(fuzz_json_parser PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_json_parser PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_json_parser PRIVATE ${FUZZ_LINK_FLAGS})
        target_link_libraries(fuzz_json_parser PRIVATE Crow::Crow)
        target_include_directories(fuzz_json_parser PRIVATE ${CMAKE_SOURCE_DIR})

        add_executable(fuzz_query_builder fuzz_targets/fuzz_query_builder.cpp)
        target_compile_definitions(fuzz_query_builder PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_query_builder PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_query_builder PRIVATE ${FUZZ_LINK_FLAGS})

        add_executable(fuzz_product_search fuzz_targets/fuzz_product_search.cpp)
        target_compile_definitions(fuzz_product_search PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_product_search PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_product_search PRIVATE ${FUZZ_LINK_FLAGS})

        add_executable(fuzz_url_decode fuzz_targets/fuzz_url_decode.cpp)
        target_compile_definitions(fuzz_url_decode PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_url_decode PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_url_decode PRIVATE ${FUZZ_LINK_FLAGS})

        add_executable(fuzz_json_body fuzz_targets/fuzz_json_body.cpp)
        target_compile_definitions(fuzz_json_body PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_json_body PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_json_body PRIVATE ${FUZZ_LINK_FLAGS})
        target_link_libraries(fuzz_json_body PRIVATE Crow::Crow)
        target_include_directories(fuzz_json_body PRIVATE ${CMAKE_SOURCE_DIR})

        add_executable(fuzz_cart_payload fuzz_targets/fuzz_cart_payload.cpp)
        target_compile_definitions(fuzz_cart_payload PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_cart_payload PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_cart_payload PRIVATE ${FUZZ_LINK_FLAGS})
        target_link_libraries(fuzz_cart_payload PRIVATE Crow::Crow)
        target_include_directories(fuzz_cart_payload PRIVATE ${CMAKE_SOURCE_DIR})

        add_executable(fuzz_search_term fuzz_targets/fuzz_search_term.cpp)
        target_compile_definitions(fuzz_search_term PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_search_term PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_search_term PRIVATE ${FUZZ_LINK_FLAGS})

        add_executable(fuzz_cookie_parser fuzz_targets/fuzz_cookie_parser.cpp)
        target_compile_definitions(fuzz_cookie_parser PRIVATE FUZZING_BUILD_MODE)
        target_compile_options(fuzz_cookie_parser PRIVATE ${FUZZ_FLAGS})
        target_link_options(fuzz_cookie_parser PRIVATE ${FUZZ_LINK_FLAGS})
    else()
        message(WARNING "BUILD_FUZZ_TARGETS requires Clang (CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}); fuzz targets skipped")
    endif()
endif()
