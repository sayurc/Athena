project(
  'athena',
  ['c'],
  license: 'GPLv3+',
  license_files: 'LICENSE',
  version: '0.1',
  default_options: [
    'c_std=c23',
    'warning_level=3',
    'b_lto=true',
  ],
)

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(
  [
  '-Wpedantic',
  '-pedantic-errors',
  '-Wmissing-prototypes',
  '-Wmissing-declarations',
  '-Wwrite-strings',
  '-Wshadow',
  '-Winline',
  '-Wnested-externs',
  '-Wfloat-equal',
  '-Wundef',
  '-Wcast-align=strict',
  '-Wpragmas',
  '-Wunreachable-code',
  '-Wdiv-by-zero',
  '-Wconversion',
  '-Wmissing-field-initializers',
  '-Wmissing-variable-declarations',
  '-Wsign-conversion',
  '-Wredundant-decls',
  '-Wheader-guard',
  ]),
  language: 'c',
)

thread_dep = dependency('threads')
m_dep = cc.find_library('m', required: false)

incdir = include_directories('include')
subdir('src')

executable(
  'athena',
  source_files,
  include_directories: incdir,
  dependencies: [thread_dep, m_dep],
  install: true)

unity = dependency('unity', required : false)
if unity.found()
  test_athena = executable(
    'test_athena',
    source_files,
    include_directories: incdir,
    dependencies: [thread_dep, m_dep, unity],
    c_args: ['-DTEST'])
  test('Test Athena', test_athena)
endif

install_man('docs/athena.1')
