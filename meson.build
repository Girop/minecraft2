project('renderer', 'cpp', default_options: ['cpp_std=c++20'])

add_project_arguments(
  '-DVK_PROTOTYPES',
  '-DVK_USE_PLATFORM_WIN32_KHR',
  language: 'cpp'
)

cpp_args = [
  '-Og',
  '-ggdb3',
  '-Wall',
  '-Wextra',
]


fmt = subproject('fmt', default_options: 'default_library=static')
fmt_dep = fmt.get_variable('fmt_dep')
glm_dep = dependency('glm')

vulkan_dep = dependency('vulkan')

deps = [glm_dep, fmt_dep, vulkan_dep]


sources = files('src/main.cpp', 'src/window.cpp')

executable(
  'renderer',
  sources,
  cpp_args: cpp_args,
  dependencies: deps
)
