project(
  'renderer',
  'cpp',
  default_options: ['cpp_std=c++20']
)


if host_machine.system() == 'windows'

add_project_arguments(
  '-DVK_USE_PLATFORM_WIN32_KHR',
  language: 'cpp',
)
endif 

add_project_arguments(
  '-DVK_PROTOTYPES',
  '-DGLFW_INCLUDE_VULKAN',
  language: 'cpp'
)

cpp_args = [
  '-Og',
  '-ggdb3',
  '-Wall',
  '-Wextra',
]

fmt = subproject('fmt', default_options: 'default_library=static')

deps = [
  dependency('glm'),
  fmt.get_variable('fmt_dep'),
  dependency('vulkan'),
  dependency('glfw3'),
  dependency('magic_enum'),
]

sources = files(
  'src/main.cpp'
)

# subdir('shaders')

executable(
  'renderer',
  sources,
  cpp_args: cpp_args,
  dependencies: deps
)
