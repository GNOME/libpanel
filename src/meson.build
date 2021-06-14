lib_header_dir = join_paths(get_option('includedir'), 'libpanel-@0@'.format(ABI_MAJOR))
libpanel_include_directories = [
  include_directories('.'),
  include_directories('..'),
]

version_data = configuration_data()
version_data.set('ABI_MAJOR', ABI_MAJOR)
version_data.set('ABI_MINOR', ABI_MINOR)
version_data.set('ABI_MICRO', ABI_MICRO)
version_data.set('VERSION', meson.project_version())

panel_version_h = configure_file(
          input: 'panel-version.h.in',
         output: 'panel-version.h',
    install_dir: lib_header_dir,
        install: true,
  configuration: version_data)

libpanel_sources = [
  'panel-binding-group.c',
  'panel-dock.c',
  'panel-dock-child.c',
  'panel-dock-switcher.c',
  'panel-frame.c',
  'panel-frame-header.c',
  'panel-frame-header-bar.c',
  'panel-frame-header-bar-row.c',
  'panel-frame-switcher.c',
  'panel-frame-tab-bar.c',
  'panel-grid.c',
  'panel-grid-column.c',
  'panel-handle.c',
  'panel-init.c',
  'panel-joined-menu.c',
  'panel-maximized-controls.c',
  'panel-paned.c',
  'panel-resizer.c',
  'panel-save-delegate.c',
  'panel-save-dialog.c',
  'panel-scaler.c',
  'panel-signal-group.c',
  'panel-widget.c',
]

libpanel_headers = [
  'libpanel.h',
  'panel-init.h',
  'panel-dock.h',
  'panel-dock-switcher.h',
  'panel-frame.h',
  'panel-frame-header.h',
  'panel-frame-header-bar.h',
  'panel-frame-switcher.h',
  'panel-frame-tab-bar.h',
  'panel-grid.h',
  'panel-grid-column.h',
  'panel-save-delegate.h',
  'panel-version-macros.h',
  'panel-widget.h',
]

libpanel_gresources = gnome.compile_resources(
  'panel-resources',
  'libpanel.gresource.xml',
  c_name: 'panel'
)

libpanel_generated_sources = [
  libpanel_gresources[0],
]

libpanel_generated_headers = [
  panel_version_h,
  libpanel_gresources[1],
]

libpanel_deps = [
  libglib_dep,
  libgtk_dep,
  libadwaita_dep,
]

libpanel_static = static_library('panel',
                                 libpanel_generated_headers +
                                 libpanel_generated_sources +
                                 libpanel_sources,
    include_directories: libpanel_include_directories,
           dependencies: libpanel_deps,
  gnu_symbol_visibility: 'hidden',
              link_args: release_link_args,
)

libpanel_static_dep = declare_dependency(
         dependencies: libpanel_deps,
           link_whole: libpanel_static,
  include_directories: libpanel_include_directories,
)

libpanel = shared_library('panel-@0@'.format(ABI_MAJOR),
           dependencies: libpanel_deps + [libpanel_static_dep],
                install: true,
  gnu_symbol_visibility: 'hidden',
                version: '@0@.0.0'.format(ABI_MAJOR),
        darwin_versions: '1.0',
              link_args: release_link_args,
)

libpanel_dep = declare_dependency(
              sources: libpanel_generated_headers,
         dependencies: libpanel_deps,
  include_directories: libpanel_include_directories,
            link_with: libpanel,
)

libpanel_pkg_reqs = [
  'glib-2.0 @0@'.format(glib_req),
  'gobject-2.0 @0@'.format(glib_req),
  'gio-2.0 @0@'.format(glib_req),
  'gtk4 @0@'.format(gtk_req),
]

libpanel_pc = pkgconfig.generate(libpanel,
         name: 'libpanel-@0@'.format(ABI_MAJOR),
  description: 'Panel library for GTK 4',
     requires: libpanel_pkg_reqs,
      version: meson.project_version(),
      subdirs: 'libpanel-@0@'.format(ABI_MAJOR),
  install_dir: pkgconfigdir,
)

install_headers(libpanel_headers,
                subdir: 'libpanel-@0@'.format(ABI_MAJOR))
