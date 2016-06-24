sample_src = [
    'WriterApp.cpp'
]

sample_exe = executable('WriterApp', 'WriterApp.cpp', 
dependencies : date_dep, include_directories : [proj_inc_dir], 
        cpp_args: cpp_flags)

sample_exe = executable('ReaderApp', 'ReaderApp.cpp', 
dependencies : date_dep, include_directories : [proj_inc_dir],
        cpp_args: cpp_flags)
