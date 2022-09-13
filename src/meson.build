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

libpanel_private_sources = [
  'gsettings-mapping.c',
  'panel-action-muxer.c',
  'panel-dock-child.c',
  'panel-drop-controls.c',
  'panel-frame-header-bar-row.c',
  'panel-handle.c',
  'panel-joined-menu.c',
  'panel-maximized-controls.c',
  'panel-resizer.c',
  'panel-save-dialog-row.c',
  'panel-scaler.c',
]

libpanel_sources = [
  'panel-dock.c',
  'panel-frame.c',
  'panel-frame-header.c',
  'panel-frame-header-bar.c',
  'panel-frame-switcher.c',
  'panel-frame-tab-bar.c',
  'panel-grid.c',
  'panel-grid-column.c',
  'panel-init.c',
  'panel-layout.c',
  'panel-layout-item.c',
  'panel-omni-bar.c',
  'panel-paned.c',
  'panel-position.c',
  'panel-save-dialog.c',
  'panel-save-delegate.c',
  'panel-statusbar.c',
  'panel-theme-selector.c',
  'panel-toggle-button.c',
  'panel-widget.c',
]

libpanel_headers = [
  'libpanel.h',
  'panel-dock.h',
  'panel-frame.h',
  'panel-frame-header-bar.h',
  'panel-frame-header.h',
  'panel-frame-switcher.h',
  'panel-frame-tab-bar.h',
  'panel-grid.h',
  'panel-grid-column.h',
  'panel-init.h',
  'panel-layout.h',
  'panel-layout-item.h',
  'panel-omni-bar.h',
  'panel-paned.h',
  'panel-position.h',
  'panel-save-delegate.h',
  'panel-statusbar.h',
  'panel-theme-selector.h',
  'panel-toggle-button.h',
  'panel-version-macros.h',
  'panel-types.h',
  'panel-widget.h',
]

panel_enum_headers = [
  'panel-types.h',
]

libpanel_enums = gnome.mkenums_simple('panel-enums',
     body_prefix: '#include "config.h"',
   header_prefix: '#include "panel-version-macros.h"',
       decorator: '_PANEL_EXTERN',
         sources: panel_enum_headers,
  install_header: true,
     install_dir: lib_header_dir,
)

libpanel_gresources = gnome.compile_resources(
  'panel-resources',
  'libpanel.gresource.xml',
  c_name: 'panel'
)

libpanel_generated_sources = [
  libpanel_gresources[0],
  libpanel_enums[0],
]

libpanel_generated_headers = [
  panel_version_h,
  libpanel_gresources[1],
  libpanel_enums[1],
]

libpanel_deps = [
  libglib_dep,
  libgtk_dep,
  libadwaita_dep,
]

libpanel_static = static_library('panel',
                                 libpanel_generated_headers +
                                 libpanel_generated_sources +
                                 libpanel_private_sources +
                                 libpanel_sources,
    include_directories: libpanel_include_directories,
           dependencies: libpanel_deps,
  gnu_symbol_visibility: 'hidden',
              link_args: release_link_args,
                 c_args: deprecation_args,
)

libpanel_static_dep = declare_dependency(
              sources: libpanel_generated_headers,
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
                 c_args: deprecation_args,
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

if generate_gir
  libpanel_gir = gnome.generate_gir(libpanel,
                sources: libpanel_generated_headers +
                         libpanel_generated_sources +
                         libpanel_headers +
                         libpanel_sources,
              nsversion: ABI_MAJOR,
              namespace: 'Panel',
        export_packages: 'libpanel-@0@'.format(ABI_MAJOR),
                 header: 'libpanel.h',
          symbol_prefix: 'panel',
      identifier_prefix: 'Panel',
              link_with: libpanel,
    include_directories: [include_directories('.')],
               includes: ['Gtk-4.0', 'Adw-1'],
                install: true,
  )
  
  if generate_vapi
    gtksource_vapi = gnome.generate_vapi('libpanel-@0@'.format(ABI_MAJOR),
          sources: libpanel_gir.get(0),
          install: true,
      install_dir: vapidir,
         packages: [ 'gio-2.0',
                     'gtk4',
                     'libadwaita-1'
                   ],
    )
  endif
endif
