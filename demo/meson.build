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
    c_name: 'demo'
  ),
]

demo = executable('libpanel-demo', demo_sources,
  dependencies: demo_dependencies,
)
