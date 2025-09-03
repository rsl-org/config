project(rsl)

function(DEFINE_EXAMPLE TARGET)
  add_executable(example_${TARGET} "${TARGET}.cpp")
  target_link_libraries(example_${TARGET} PRIVATE rsl-config)
endfunction()

DEFINE_EXAMPLE(simple)
DEFINE_EXAMPLE(stateful)
DEFINE_EXAMPLE(extension)
