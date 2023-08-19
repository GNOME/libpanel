if get_option('docs').allowed() and get_option('introspection').allowed() 
  dependency('gi-docgen', version: '>= 2021.1',
           fallback: ['gi-docgen', 'dummy_dep'],
           native: true,
           required: get_option('docs'))

  gidocgen = find_program('gi-docgen')

  docdir = datadir / 'doc'

  toml_conf = configuration_data()
  toml_conf.set('PANEL_VERSION', meson.project_version())

  libpanel_toml = configure_file(
    input: 'libpanel.toml.in',
    output: 'libpanel.toml',
    configuration: toml_conf,
  )

  custom_target('panel-doc',
    input:  libpanel_gir[0],
    output: 'panel-1.0',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--fatal-warnings',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../src'),
      '--config', libpanel_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [
      libpanel_toml,
      'introduction.md'
    ],
    build_by_default: true,
    install: true,
    install_dir: docdir,
  )
elif get_option('docs').allowed() and get_option('introspection').disabled() 
  error('docs require introspection')
endif
