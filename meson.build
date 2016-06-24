project('serialization', 'cpp')

date_sub = subproject('date')
date_dep = date_sub.get_variable('date_dep')

boost_dep = dependency('boost', modules : ['fusion', 'asio', 'endian'])
proj_inc_dir = include_directories('include')


cpp_flags = ['-std=c++14']

serialize_dep = declare_dependency(include_directories : proj_inc_dir,
  dependencies : boost_dep)
subdir('test')
subdir('samples')

