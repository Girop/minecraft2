glslc = find_program('glslc', required: true)
# Fixme?

shader_compiler = generator(
  glslc,
  arguments: ['-o', '@OUTPUT@', '@INPUT@'],
  output: '@BASENAME@.spv'
)

shaders = files(
  'triangle_frag.frag',
  'triangle_vert.vert',
)

compiled_shaders = shader_compiler.process(shaders)
custom_target()
