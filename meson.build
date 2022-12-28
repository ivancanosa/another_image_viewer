project('main', 'cpp', default_options: 'default_library=static')

add_global_arguments('-std=c++17', language : 'cpp')
add_global_arguments('-stdlib=libc++', language : 'cpp')
add_global_arguments('-Wno-everything', language : 'cpp')

incdir = include_directories('.')

sdl2_dep = dependency('sdl2')
sdl2_image_dep = dependency('sdl2_image')
sdl2_ttf_dep = dependency('sdl2_ttf')

all_deps = [sdl2_dep, sdl2_image_dep, sdl2_ttf_dep]

if host_machine.system() != 'windows'
  fontconfig_dep = dependency('fontconfig', required: true)
  all_deps += fontconfig_dep
endif

subdir('tests')

executable('aiv', 'aiv.cpp', dependencies: all_deps)


