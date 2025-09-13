project('libpanel', 'c',
          version: '1.10.3',
          license: 'LGPL-3.0-or-later',
    meson_version: '>= 0.63',
  default_options: [ 'warning_level=2', 'c_std=gnu11,c11' ],
)

gnome = import('gnome')
pkgconfig = import('pkgconfig')
i18n = import('i18n')
cc = meson.get_compiler('c')
buildtype = get_option('buildtype')

VERSION = meson.project_version()
ABI_MAJOR = VERSION.split('.')[0]
ABI_MINOR = VERSION.split('.')[1]
ABI_MICRO = VERSION.split('.')[2]
if ABI_MICRO.startswith('alpha') or ABI_MICRO.startswith('beta')
  ABI_MICRO  = '0'
endif

# Setup commonly used options
prefix = get_option('prefix')
libdir = join_paths(prefix, get_option('libdir'))
pkgconfigdir = join_paths(libdir, 'pkgconfig')
datadir = join_paths(prefix, get_option('datadir'))
vapidir = join_paths(datadir, 'vala', 'vapi')

# Check dependencies
glib_req_version = '2.80'
gtk_req_version = '4.15'
adwaita_req_version = '1.6'

glib_req = '>= @0@'.format(glib_req_version)
gtk_req = '>= @0@'.format(gtk_req_version)
adwaita_req = '>= @0@'.format(adwaita_req_version)

libglib_dep = dependency('gio-2.0', version: glib_req)
libgtk_dep = dependency('gtk4', version: gtk_req)
libadwaita_dep = dependency('libadwaita-1', version: adwaita_req)

# Setup commonly used config data
config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', 'libpanel')
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'libpanel')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))

if get_option('development')
  config_h.set10('DEVELOPMENT_BUILD', true)
endif

# Specify minimum library versions
glib_major = glib_req_version.split('.')[0].to_int()
glib_minor = glib_req_version.split('.')[1].to_int()
gtk_major = gtk_req_version.split('.')[0].to_int()
gtk_minor = gtk_req_version.split('.')[1].to_int()

if glib_minor % 2 == 1
  glib_minor = glib_minor + 1
endif
if gtk_minor % 2 == 1
  gtk_minor = gtk_minor + 1
endif

# Compiler flags and warnings
deprecation_args = [
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
]
global_c_args = [
  '-DLIBPANEL_COMPILATION',
  '-DG_LOG_DOMAIN="libpanel"',
]
if cc.get_id() == 'msvc'
  test_c_args = [
    '/FImsvc_recommended_pragmas.h',
  ]
else
  test_c_args = [
    '-Wcast-align',
    '-Werror=address',
    '-Werror=array-bounds',
    '-Werror=empty-body',
    '-Werror=implicit',
    '-Werror=implicit-function-declaration',
    '-Werror=init-self',
    '-Werror=int-conversion',
    '-Werror=int-to-pointer-cast',
    '-Werror=main',
    '-Werror=misleading-indentation',
    '-Werror=missing-braces',
    '-Werror=missing-include-dirs',
    '-Werror=nonnull',
    '-Werror=overflow',
    '-Werror=parenthesis',
    '-Werror=pointer-arith',
    '-Werror=pointer-to-int-cast',
    '-Werror=return-type',
    '-Werror=sequence-point',
    '-Werror=shadow',
    '-Werror=strict-prototypes',
    '-Werror=trigraphs',
    '-Werror=undef',
    '-Werror=write-strings',
    '-Wformat-nonliteral',
    ['-Werror=format-security', '-Werror=format=2' ],
    '-Wignored-qualifiers',
    '-Wincompatible-pointer-types',
    '-Wimplicit-function-declaration',
    '-Wlogical-op',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wnested-externs',
    '-Wno-cast-function-type',
    '-Wno-missing-field-initializers',
    '-Wno-sign-compare',
    '-Wno-unused-parameter',
    '-Wno-typedef-redefinition',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wuninitialized',
    '-Wunused',
    '-fno-strict-aliasing',
  ]
  # Ignore some Werror for clang until GLib fixes volatile usage in
  # GType registration.
  if meson.get_compiler('c').get_id() == 'clang'
    test_c_args += ['-Wno-incompatible-pointer-types-discards-qualifiers']
  endif

  if buildtype != 'plain'
    test_c_args += '-fstack-protector-strong'
  endif
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach

if cc.get_id() != 'msvc' and cc.has_multi_arguments('-Wmissing-declarations')
  global_c_args += '-Wmissing-declarations'
endif

add_project_arguments(global_c_args, language: 'c')

# Detect and set symbol visibility
if get_option('default_library') != 'static'
  if host_machine.system() == 'windows'
    config_h.set('DLL_EXPORT', true)
    if cc.get_id() == 'msvc'
      config_h.set('_PANEL_EXTERN', '__declspec(dllexport) extern')
    elif cc.has_argument('-fvisibility=hidden')
      config_h.set('_PANEL_EXTERN', '__attribute__((visibility("default"))) __declspec(dllexport) extern')
    endif
  elif cc.has_argument('-fvisibility=hidden')
    config_h.set('_PANEL_EXTERN', '__attribute__((visibility("default"))) extern')
  endif
endif

# GObject introspection Options
gir = find_program('g-ir-scanner', required : get_option('introspection'))
generate_gir = gir.found() and (not meson.is_cross_build() or get_option('introspection').enabled())
generate_vapi = generate_gir and get_option('vapi')

# Various optimizations for non-debug builds including disabling of
# cast checks, asserts, and additional link options.
release_link_args = []
test_link_args = [
  '-Wl,-z,defs',   # Detect and reject underlinking
  '-Wl,-z,now',    # Disable lazy binding
  '-Wl,-z,relro',  # Read-only segments after relocation
]
if not buildtype.startswith('debug')
  add_global_arguments([
    '-DG_DISABLE_ASSERT',
    '-DG_DISABLE_CAST_CHECKS',
  ], language: 'c')
  test_link_args += ['-Wl,-Bsymbolic', '-fno-plt']
endif
foreach link_arg: test_link_args
  if cc.has_link_argument(link_arg)
    release_link_args += link_arg
  endif
endforeach

configure_file(
         output: 'config.h',
  configuration: config_h,
)

subdir('data')
subdir('src')
subdir('example')
subdir('demo')
subdir('testsuite')
subdir('po')
subdir('docs')

gnome.post_install(gtk_update_icon_cache: true)
