project('avsegy', 'c',
  version : '0.0.1',
  license : 'LGPL',
  default_options : ['c_std=c11',
    'buildtype=release',
    'b_ndebug=if-release',
    'warning_level=3'
  ]
  )
inc = include_directories('include')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : true)
subdir('test')

py = import('python').find_installation(pure: false)
numpy_inc = run_command(
  py,
  ['-c', 'import numpy; print(numpy.get_include())'],
  check : true,
).stdout().strip()

py.extension_module(
  'avsegy',
  sources : ['src/avsegy.c', 'src/AVSegyCommon.c',
    'src/AVSegyReader.c', 'src/AVSegyUtils.c',
    'src/PyAVSegyBinaryHeader.c', 'src/PyAVSegyReader.c',
    'src/PyAVSegyError.c'],
  include_directories : [numpy_inc, inc],
  install : true,

  c_args : ['-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION'],
)
