libpanel_example_sources = [
  'example-page.c',
  'example-window.c',
  'main.c',
]

libpanel_example_gresources = gnome.compile_resources(
  'example-resources',
  'libpanel-example.gresource.xml',
  c_name: 'example'
)

gtksourceview_dep = dependency('gtksourceview-5', version: '>= 5.3', required: false)

if gtksourceview_dep.found()
  libpanel_example_cargs = ['-DHAVE_GTKSOURCEVIEW']
else
  libpanel_example_cargs = []
endif

libpanel_example = executable('libpanel-example',
  libpanel_example_sources + libpanel_example_gresources,
  dependencies: [libpanel_dep, gtksourceview_dep],
  build_by_default: not meson.is_subproject(),
  install: get_option('install-examples'),
  c_args: libpanel_example_cargs + deprecation_args,
)
