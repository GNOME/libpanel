
# subproject build breaks if demos are included
if meson.is_subproject()
  subdir_done()
endif

demo_dependencies = [
  libpanel_dep,
]

demo_sources = [
  'main.c',
  'demo-page.c',
  'demo-workspace.c',

  gnome.compile_resources(
    'panel-resources',
    'demo.gresource.xml',
    c_name: 'panel'
  ),
]

demo = executable('libpanel-demo', demo_sources,
  dependencies: demo_dependencies,
)
