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
m_dep = cc.find_library('m', required: true)

incdir = include_directories('include')
subdir('src')

executable(
  'athena',
  source_files,
  include_directories: incdir,
  dependencies: [thread_dep, m_dep],
  install: true)

unity = dependency('unity')
test_eval = executable(
  'test_eval',
  test_source_files,
  include_directories: incdir,
  dependencies: [thread_dep, m_dep, unity],
  c_args: ['-DTEST', '-DTEST_EVAL'])
test_movegen = executable(
  'test_movegen',
  test_source_files,
  include_directories: incdir,
  dependencies: [thread_dep, m_dep, unity],
  c_args: ['-DTEST', '-DTEST_MOVEGEN'])
test('Test eval.c', test_eval)
test('Test movegen.c', test_movegen)

install_man('docs/athena.1')
