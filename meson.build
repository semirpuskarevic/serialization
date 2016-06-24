project('serialization', 'cpp')

date_sub = subproject('date')
date_dep = date_sub.get_variable('date_dep')

boost_deps = dependency('boost', modules : ['fusion', 'asio', 'endian'])
proj_inc_dir = include_directories('include')


cpp_flags = ['-std=c++14']

subdir('test')
subdir('samples')
