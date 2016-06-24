gtest = dependency('gtest', main : false)
gmock = dependency('gmock')

test_src = [
    'main.cpp',
    'ReaderTest.cpp',
    'WriterTest.cpp',
    'NetworkTest.cpp',
    'SizerTest.cpp',
    'LazyTypeTest.cpp'
]

test_exe = executable('SerializationTests', test_src, include_directories : [proj_inc_dir], 
dependencies : [gtest, gmock, date_dep], 
cpp_args: cpp_flags)

#dir_name = '@0@/@1@'.format(meson.source_root(), 'config')

test('serialization test', test_exe)
