add_executable(lab04_parent src/parent.c)
add_executable(lab04_child src/child.c)

target_compile_definitions(lab04_parent PRIVATE
        CHILD_PATH=\"$<TARGET_FILE:lab04_child>\"
)

add_dependencies(lab04_parent lab04_child)
