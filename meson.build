project('renderer', 'cpp', default_options: ['cpp_std=c++20'])

if host_machine.system() == 'windows'
add_project_arguments(
  '-DVK_USE_PLATFORM_WIN32_KHR',
  language: 'cpp',
)
endif 

add_project_arguments(
  '-DVK_PROTOTYPES',
  '-DGLFW_INCLUDE_VULKAN',
  '-DGLM_FORCE_RADIANS ',
  '-DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES',
  '-DGLM_FORCE_DEPTH_ZERO_TO_ONE',
  language: 'cpp'
)

cpp_args = [
  '-Og',
  '-ggdb3',
  '-Wall',
  '-Wextra',
]

subdir('shaders')

deps = [
  dependency('glm'),
  subproject('fmt', default_options: 'default_library=static').get_variable('fmt_dep'),
  dependency('vulkan'),
  dependency('glfw3', static: true, method: 'pkg-config'),
  shader_dep,
  dependency('stb')
]

sources = files(
  'src/main.cpp',
  'src/engine.cpp',
  'src/swapchain.cpp',
  'src/shader.cpp',
  'src/pipeline.cpp',
  'src/window.cpp',
  'src/uniforms.cpp',
  'src/device.cpp',
  'src/camera.cpp',
  'src/vertex.cpp'
)

executable(
  'renderer',
  sources,
  cpp_args: cpp_args,
  dependencies: deps,
)
