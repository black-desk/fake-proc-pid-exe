project(
  'attack',
  'cpp',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++11',
  ],
)

dep_list = {
  'fmt': 'fmt_dep',
}

deps = [
  dependency('dbus-1'),
]
foreach dep_name, var_name: dep_list
  d = dependency(dep_name, required : false)
  if not d.found()
    d = subproject(dep_name).get_variable(var_name)
  endif
  deps += d
endforeach

executable(
  'attack',
  'src/main.cpp',
  'src/cmd/run.cpp',
  'src/cmd/send.cpp',
  'src/cmd/server.cpp',
  dependencies : deps,
)
