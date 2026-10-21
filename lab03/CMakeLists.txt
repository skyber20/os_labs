find_package(Threads REQUIRED)

add_executable(lab03 src/main.c)
target_link_libraries(lab03 PRIVATE Threads::Threads)

add_executable(lab03_bench src/bench.c)
