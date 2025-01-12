fs = import('fs')

shader_targets = []
shader_compiler = find_program('glslc')
shader_files = files('triangle.frag', 'triangle.vert')

foreach shader : shader_files
  shader_targets += custom_target(
    fs.name(shader),
    input : shader,
    output : fs.name(shader) + '.spv',
    command : ['glslc', '@INPUT@', '-o', '@OUTPUT@'],
)
endforeach


shader_dep = declare_dependency(sources: shader_targets)
