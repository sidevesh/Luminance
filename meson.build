project('com.sidevesh.Luminance', 'c',
  version: run_command('cat', 'version.txt', check: true).stdout().strip(),
  default_options: [
    'warning_level=3',
    'c_std=gnu11'
  ]
)

gnome = import('gnome')

# Dependencies
gtk4_dep = dependency('gtk4', required: true)
libadwaita_dep = dependency('libadwaita-1', required: true)
ddcutil_dep = dependency('ddcutil', required: true)
thread_dep = dependency('threads')

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('APP_INFO_VERSION_NUMBER', meson.project_version())

# Application ID and Icon determination
if get_option('buildtype').startswith('debug')
  app_id = 'com.sidevesh.Luminance.Devel'
  icon_name = 'com.sidevesh.Luminance.Devel'
else
  app_id = 'com.sidevesh.Luminance'
  icon_name = 'com.sidevesh.Luminance'
endif

conf_data.set_quoted('APP_INFO_PACKAGE_NAME', app_id)
conf_data.set_quoted('APP_INFO_ICON_NAME', icon_name)

configure_file(output: 'config.h', configuration: conf_data)

# Main executable
# Note: src/main.c includes other source files directly.
# This structure means we only need to compile main.c.
executable(app_id,
  sources: 'src/main.c',
  dependencies: [
    gtk4_dep, 
    libadwaita_dep, 
    ddcutil_dep,
    thread_dep
  ],
  install: true,
  install_dir: get_option('bindir')
)

# Installation of data files
datadir = get_option('datadir')

# Desktop file
desktop_conf = configuration_data()
desktop_conf.set('APP_ID', app_id)
desktop_conf.set('ICON_NAME', icon_name)
desktop_conf.set('EXEC_NAME', app_id)

configure_file(
  input: 'install_files/com.sidevesh.Luminance.desktop.in',
  output: app_id + '.desktop',
  configuration: desktop_conf,
  install: true,
  install_dir: join_paths(datadir, 'applications')
)

# GSchema
schema_conf = configuration_data()
schema_conf.set('APP_ID', app_id)
# Construct path like /com/sidevesh/Luminance/ or /com/sidevesh/Luminance/Devel/
schema_path = '/' + app_id.replace('.', '/') + '/'
schema_conf.set('SCHEMA_PATH', schema_path)

configure_file(
  input: 'install_files/com.sidevesh.Luminance.gschema.xml.in',
  output: app_id + '.gschema.xml',
  configuration: schema_conf,
  install: true,
  install_dir: join_paths(datadir, 'glib-2.0/schemas')
)

# Udev rules
# Try to detect udev rules directory, otherwise fallback to /lib/udev/rules.d
udev_dep = dependency('udev', required: false)
if udev_dep.found()
  udev_rules_dir = udev_dep.get_pkgconfig_variable('udevdir') + '/rules.d'
else
  # Standard location for udev rules
  udev_rules_dir = '/lib/udev/rules.d'
endif

install_data('install_files/44-backlight-permissions.rules',
  install_dir: udev_rules_dir
)

# Icons
# Install the appropriate icon based on the build type
if icon_name == 'com.sidevesh.Luminance.Devel'
  install_data('icons/hicolor/scalable/apps/com.sidevesh.Luminance.Devel.svg',
    install_dir: join_paths(datadir, 'icons/hicolor/scalable/apps'),
    rename: icon_name + '.svg' 
  )
  install_data('icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg',
      install_dir: join_paths(datadir, 'icons/hicolor/symbolic/apps')
  )
else
  install_data('icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg',
    install_dir: join_paths(datadir, 'icons/hicolor/scalable/apps')
  )
  install_data('icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg',
    install_dir: join_paths(datadir, 'icons/hicolor/symbolic/apps')
  )
endif

# Post-install scripts
gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
