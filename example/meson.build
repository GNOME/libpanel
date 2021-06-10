libpanel_example_sources = [
  'example-window.c',
  'main.c',
]

libpanel_example_gresources = gnome.compile_resources(
  'example-resources',
  'libpanel-example.gresource.xml',
  c_name: 'example'
)

libpanel_example = executable('libpanel-example',
  libpanel_example_sources + libpanel_example_gresources,
  dependencies: [libpanel_dep],
  install: get_option('install-examples'),
)
