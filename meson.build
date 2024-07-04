project('luminance', 'c',
  version : run_command('cat', 'version.txt', check: true).stdout().strip(),
  default_options : ['warning_level=3', 'c_std=c11']
)

gnome = import('gnome')

# Get the version (already set in project, but we need it for conf_data)
version = meson.project_version()

# Check if this is a development build
is_devel = get_option('buildtype').startswith('debug')

# Set the application ID
if is_devel
  application_id = 'com.sidevesh.Luminance.Devel'
else
  application_id = 'com.sidevesh.Luminance'
endif

# Convert APPLICATION_ID to APPLICATION_PATH
application_path = '/' + '/'.join(application_id.split('.')) + '/'

# Configuration data for C code (config.h)
conf_data_c = configuration_data()
conf_data_c.set_quoted('APPLICATION_ID', application_id)
conf_data_c.set_quoted('APPLICATION_VERSION', version)
if is_devel
  conf_data_c.set('DEVELOPMENT_BUILD', true)
endif

# Configuration data for other files (desktop, schema, etc.)
conf_data = configuration_data()
conf_data.set('APPLICATION_ID', application_id)
conf_data.set('APPLICATION_PATH', application_path)

# Generate config.h
configure_file(
  output: 'config.h',
  configuration: conf_data_c
)

# Dependencies
gtk_dep = dependency('gtk4')
libadwaita_dep = dependency('libadwaita-1')
ddcutil_dep = dependency('ddcutil')

# Source files
src_files = [
  'src/main.c',
]

# Include directories
inc = include_directories('src')

# Executable
executable(application_id,
  src_files,
  dependencies : [gtk_dep, libadwaita_dep, ddcutil_dep],
  include_directories : inc,
  install : true,
)

# Install desktop file
desktop_file = configure_file(
  input: 'install_files/com.sidevesh.Luminance.desktop.in',
  output: application_id + '.desktop',
  configuration: conf_data,
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

# Process and install GSchema
schema_file = configure_file(
  input: 'install_files/com.sidevesh.Luminance.gschema.xml.in',
  output: application_id + '.gschema.xml',
  configuration: conf_data,
  install: true,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)

# Compile schemas
gnome.compile_schemas(
  build_by_default: true,
  depend_files: schema_file
)

# Install udev rules
install_data('install_files/44-backlight-permissions.rules',
  install_dir : get_option('sysconfdir') / 'udev' / 'rules.d'
)

# Install icons
if is_devel
  install_data('icons/hicolor/scalable/apps/com.sidevesh.Luminance.Devel.svg',
    install_dir : get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
    rename : application_id + '.svg'
  )
  install_data('icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg',
    install_dir : get_option('datadir') / 'icons' / 'hicolor' / 'symbolic' / 'apps',
    rename : application_id + '-symbolic.svg'
  )
else
  install_data('icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg',
    install_dir : get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
  )
  install_data('icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg',
    install_dir : get_option('datadir') / 'icons' / 'hicolor' / 'symbolic' / 'apps'
  )
endif

# Post-install script
meson.add_install_script('meson_post_install.sh')
