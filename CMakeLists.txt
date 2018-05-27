cmake_minimum_required(VERSION 3.9)
project(Banshee)
set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_subdirectory(third_party/cedilla)

add_library(afio INTERFACE)
target_include_directories(afio INTERFACE third_party/afio/include)

add_library(stdc++fs INTERFACE)
target_link_libraries(stdc++fs INTERFACE "-lstdc++fs" Threads::Threads ${CMAKE_DL_LIBS})

target_link_libraries(afio INTERFACE stdc++fs)


add_library( banshee
    include/banshee/banshee.hpp
    include/banshee/unicode.hpp
    include/banshee/unicode_view.hpp
    include/banshee/lexer.hpp
    include/banshee/parser.hpp
    include/banshee/property.hpp
    include/banshee/json/json_lexer.hpp
    include/banshee/json/json_parser.hpp
    include/banshee/detail/unicode_file.hpp
    include/banshee/detail/generator.hpp
    include/banshee/detail/util.hpp
    src/fix_bad_access.cpp
)
target_link_libraries(banshee PUBLIC afio cedilla stdc++fs c++)
target_include_directories(banshee PUBLIC include)
target_compile_options(banshee PUBLIC -fcoroutines-ts -stdlib=libc++)

add_executable(banshee-test-file
    tests/file.cpp
)
target_link_libraries(banshee-test-file PUBLIC banshee)

add_executable(banshee-test-validate
    tests/validate.cpp
)

target_link_libraries(banshee-test-validate PUBLIC banshee)




