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
  'src/ext/stbi.cpp',
  'src/gfx/buffer.cpp',
  'src/gfx/commands.cpp',
  'src/gfx/descriptors.cpp',
  'src/gfx/device.cpp',
  'src/gfx/framedata.cpp',
  'src/gfx/image.cpp',
  'src/gfx/pipeline.cpp',
  'src/gfx/queues.cpp',
  'src/gfx/renderer.cpp',
  'src/gfx/shader.cpp',
  'src/gfx/swapchain.cpp',
  'src/gfx/sync.cpp',
  'src/gfx/uniforms.cpp',
  'src/gfx/vertex.cpp',
  'src/app.cpp',
  'src/camera.cpp',
  'src/input.cpp',
  'src/main.cpp',
  'src/texture.cpp',
  'src/window.cpp',
  'src/world.cpp'
)


inc_dir = include_directories('src')

executable(
  'renderer',
  sources,
  cpp_args: cpp_args,
  dependencies: deps,
  include_directories : inc_dir
)
