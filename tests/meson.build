catch2_dep = dependency('catch2-with-main')

test_sources = files(
  'test_registers.cpp',
  'test_flags.cpp',
  'test_alu.cpp',
  'test_decode.cpp',
  'test_interrupts.cpp',
  'test_block_ops.cpp',
  'test_indexed_ops.cpp',
  'test_timing.cpp',
  'test_host_examples.cpp',
  'test_zexdoc.cpp'
)

z80f_tests = executable(
  'z80f-tests',
  test_sources,
  dependencies: [z80f_dep, catch2_dep]
)

test(
  'z80f unit tests',
  z80f_tests,
  workdir: meson.project_source_root(),
  timeout: 300
)
