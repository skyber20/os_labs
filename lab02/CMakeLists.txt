add_executable(lab02_parent src/parent.c)

add_executable(lab02_child src/child.c)

target_compile_definitions(lab02_parent PRIVATE
        CHILD_PATH=\"$<TARGET_FILE:lab02_child>\"
)

add_dependencies(lab02_parent lab02_child)
