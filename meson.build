project(
  'attack',
  'cpp',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++11',
  ],
)

executable(
  'attack',
  'src/main.cpp',
  'src/cmd/run.cpp',
  'src/cmd/send.cpp',
  'src/cmd/server.cpp',
  dependencies : [
    dependency('fmt'),
    dependency('dbus-1'),
  ]
)
