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

test_exe = executable('SerializationTests', test_src, 
dependencies : [gtest, gmock, date_dep, serialize_dep], 
cpp_args: cpp_flags)

#dir_name = '@0@/@1@'.format(meson.source_root(), 'config')

test('Serialization test', test_exe)
