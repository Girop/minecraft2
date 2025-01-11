project(
  'renderer',
  'cpp',
  default_options: ['cpp_std=c++20']
)

add_project_arguments(
  '-DVK_PROTOTYPES',
  '-DVK_USE_PLATFORM_WIN32_KHR',
  '-DGLFW_INCLUDE_VULKAN',
  '-DGLM_FORCE_RADIANS ',
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
  dependency('glfw3')
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
