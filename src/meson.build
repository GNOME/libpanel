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
  'panel-dock-child.c',
  'panel-drop-controls.c',
  'panel-frame-header-bar-row.c',
  'panel-handle.c',
  'panel-joined-menu.c',
  'panel-maximized-controls.c',
  'panel-progress-icon.c',
  'panel-resizer.c',
  'panel-save-dialog-row.c',
  'panel-scaler.c',
]

libpanel_sources = [
  'panel-action-muxer.c',
  'panel-application.c',
  'panel-changes-dialog.c',
  'panel-dock.c',
  'panel-document-workspace.c',
  'panel-frame.c',
  'panel-frame-header.c',
  'panel-frame-header-bar.c',
  'panel-frame-switcher.c',
  'panel-frame-tab-bar.c',
  'panel-grid.c',
  'panel-grid-column.c',
  'panel-gsettings-action-group.c',
  'panel-inhibitor.c',
  'panel-init.c',
  'panel-layered-settings.c',
  'panel-menu-manager.c',
  'panel-omni-bar.c',
  'panel-paned.c',
  'panel-position.c',
  'panel-save-dialog.c',
  'panel-save-delegate.c',
  'panel-session.c',
  'panel-session-item.c',
  'panel-settings.c',
  'panel-statusbar.c',
  'panel-theme-selector.c',
  'panel-toggle-button.c',
  'panel-widget.c',
  'panel-workbench.c',
  'panel-workspace.c',
]

libpanel_headers = [
  'libpanel.h',
  'panel-action-group.h',
  'panel-action-muxer.h',
  'panel-application.h',
  'panel-changes-dialog.h',
  'panel-dock.h',
  'panel-document-workspace.h',
  'panel-frame.h',
  'panel-frame-header-bar.h',
  'panel-frame-header.h',
  'panel-frame-switcher.h',
  'panel-frame-tab-bar.h',
  'panel-grid.h',
  'panel-grid-column.h',
  'panel-gsettings-action-group.h',
  'panel-inhibitor.h',
  'panel-init.h',
  'panel-layered-settings.h',
  'panel-menu-manager.h',
  'panel-omni-bar.h',
  'panel-paned.h',
  'panel-position.h',
  'panel-save-dialog.h',
  'panel-save-delegate.h',
  'panel-session.h',
  'panel-session-item.h',
  'panel-settings.h',
  'panel-statusbar.h',
  'panel-theme-selector.h',
  'panel-toggle-button.h',
  'panel-version-macros.h',
  'panel-types.h',
  'panel-widget.h',
  'panel-workbench.h',
]

panel_enum_headers = [
  'panel-types.h',
]

libpanel_marshal = gnome.genmarshal('panel-marshal',
            internal: true,
              prefix: 'panel_marshal',
             sources: ['panel-marshal.defs'],
  valist_marshallers: true,
)

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
  libpanel_marshal[0],
]

libpanel_generated_headers = [
  panel_version_h,
  libpanel_gresources[1],
  libpanel_enums[1],
  libpanel_marshal[1],
]

libpanel_deps = [
  libglib_dep,
  libgtk_dep,
  libadwaita_dep,
]

libpanel_static = static_library('panel-static-@0@'.format(ABI_MAJOR),
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
            link_with: libpanel_static,
  include_directories: libpanel_include_directories,
)

libpanel = library('panel-@0@'.format(ABI_MAJOR),
           dependencies: libpanel_deps,
             link_whole: libpanel_static,
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
  libpanel_gir_extra_args = []
  if gir.version().version_compare('>=1.83.2')
    libpanel_gir_extra_args += [
      '--doc-format=gi-docgen',
    ]
  endif
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
             extra_args: libpanel_gir_extra_args,
  )

  if generate_vapi
    gtksource_vapi = gnome.generate_vapi('libpanel-@0@'.format(ABI_MAJOR),
          sources: libpanel_gir.get(0),
          install: true,
      install_dir: vapidir,
         packages: [ 'gio-2.0', 'gtk4', 'libadwaita-1' ],
        vapi_dirs: [vapidir],
    )
  endif
endif
