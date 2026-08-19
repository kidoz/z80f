project(
  'z80f',
  'cpp',
  version: '1.0.0',
  license: 'MIT',
  meson_version: '>=1.3.0',
  default_options: [
    'cpp_std=c++23,c++latest',
    'warning_level=3',
    'werror=false',
    'b_ndebug=if-release'
  ]
)

z80f_inc = include_directories('include')

z80f_sources = files(
  'src/z80.cpp',
  'src/decode.cpp',
  'src/instructions.cpp',
  'src/flags.cpp',
  'src/timing.cpp',
  'src/snapshot.cpp'
)

z80f_lib = library(
  'z80f',
  z80f_sources,
  include_directories: z80f_inc,
  version: meson.project_version(),
  install: true
)

z80f_dep = declare_dependency(
  include_directories: z80f_inc,
  link_with: z80f_lib
)

meson.override_dependency('z80f', z80f_dep)

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  z80f_lib,
  name: 'z80f',
  description: 'Cycle-aware Zilog Z80 emulation core'
)

install_headers(
  'include/z80f/z80.hpp',
  'include/z80f/bus.hpp',
  'include/z80f/registers.hpp',
  'include/z80f/flags.hpp',
  'include/z80f/snapshot.hpp',
  'include/z80f/version.hpp',
  subdir: 'z80f'
)

if get_option('tests')
  subdir('tests')
endif

if get_option('examples')
  executable(
    'zx-spectrum-host-example',
    'examples/zx_spectrum_host.cpp',
    dependencies: z80f_dep
  )

  executable(
    'megadrive-sound-z80-host-example',
    'examples/megadrive_sound_z80_host.cpp',
    dependencies: z80f_dep
  )
endif
