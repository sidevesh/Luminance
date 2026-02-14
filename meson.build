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
gio_dep = dependency('gio-2.0')
libadwaita_dep = dependency('libadwaita-1', required: true)
ddcutil_dep = dependency('ddcutil', required: true)
thread_dep = dependency('threads')

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('APP_INFO_VERSION_NUMBER', meson.project_version())
conf_data.set_quoted('APP_INFO_DESCRIPTION', run_command('cat', 'description.txt', check: true).stdout().strip())

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

# DBus Interface Generation
xml_conf = configuration_data()
xml_conf.set('APP_INFO_PACKAGE_NAME', app_id)

dbus_xml = configure_file(
    input: 'install_files/com.sidevesh.Luminance.xml.in',
    output: app_id + '.xml',
    configuration: xml_conf
)

# Generates com-sidevesh-Luminance.c and com-sidevesh-Luminance.h
gdbus_src = gnome.gdbus_codegen(
  'com-sidevesh-Luminance',
  sources: dbus_xml,
  interface_prefix: app_id + '.',
  namespace: 'Luminance',
  annotations: [[app_id, 'org.gtk.GDBus.C.Name', 'Service']]
)

# Create a static library for generated code to suppress strict warnings
gdbus_lib = static_library('gdbus_generated',
  sources: gdbus_src,
  dependencies: [gio_dep],
  override_options: ['warning_level=0']
)

# Main executable
# Note: src/main.c includes other source files directly.
# This structure means we only need to compile main.c.
executable(app_id,
  sources: ['src/main.c'],
  dependencies: [
    gtk4_dep, 
    libadwaita_dep, 
    ddcutil_dep,
    thread_dep
  ],
  link_with: gdbus_lib,
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
desktop_conf.set('DESCRIPTION', run_command('cat', 'description.txt', check: true).stdout().strip())

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

# AppStream Metadata
# Check if releases.xml exists
releases_xml_file = meson.project_source_root() / 'releases.xml'
if not import('fs').is_file(releases_xml_file)
  error('releases.xml file not found. Please generate it by running: ./scripts/generate-releases.sh')
endif

# Read releases.xml content
releases_content = run_command('cat', 'releases.xml', check: true).stdout().strip()

# Validate that the latest release version matches version.txt
# Extract the first version from releases.xml using a more robust pattern
# This matches: <release version="X.Y.Z" ... /> where X.Y.Z is the version
version_extract_result = run_command('sh', '-c', 
  'grep -m 1 "<release version=" releases.xml | sed -E \'s/.*<release[[:space:]]+version="([^"]+)".*/\\1/\'',
  check: true)
latest_release_version = version_extract_result.stdout().strip()

# Validate that we extracted a non-empty version in proper format
if latest_release_version == ''
  error('Failed to extract version from releases.xml. The file may be malformed or empty.')
endif

# Verify the version has a reasonable format (X.Y.Z where X, Y, Z are numbers, with optional suffixes)
version_format_check = run_command('sh', '-c',
  'echo "' + latest_release_version + '" | grep -qE \'^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?(-[a-zA-Z0-9]+)?$\' && echo "valid" || echo "invalid"',
  check: true)

if version_format_check.stdout().strip() != 'valid'
  error('Invalid version format in releases.xml: "' + latest_release_version + '". Expected format: X.Y.Z (e.g., 1.4.3)')
endif

if latest_release_version != meson.project_version()
  error('Version mismatch: releases.xml latest version (' + latest_release_version + ') does not match version.txt (' + meson.project_version() + ')')
endif

appstream_conf = configuration_data()
appstream_conf.set('APP_ID', app_id)
appstream_conf.set('DESCRIPTION', run_command('cat', 'description.txt', check: true).stdout().strip())
appstream_conf.set('RELEASES', releases_content)

configure_file(
  input: 'install_files/com.sidevesh.Luminance.metainfo.xml.in',
  output: app_id + '.metainfo.xml',
  configuration: appstream_conf,
  install: true,
  install_dir: join_paths(datadir, 'metainfo')
)

# DBus activatable service
dbus_service_conf = configuration_data()
dbus_service_conf.set('APP_ID', app_id)
dbus_service_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input: 'install_files/com.sidevesh.Luminance.service.in',
  output: app_id + '.service',
  configuration: dbus_service_conf,
  install: true,
  install_dir: join_paths(datadir, 'dbus-1', 'services')
)

# Udev rules
if get_option('install_udev_rules')
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
endif

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
# Check for CI environment to skip post-install scripts
is_ci = run_command('sh', '-c', 'echo $CI', check: false).stdout().strip() == 'true'

# Check for tools first to avoid build failures in environments where they are missing.
# Packagers typically handle these updates via triggers anyway.
update_desktop_db = find_program('update-desktop-database', required: false)
gtk_update_icon_cache = find_program('gtk4-update-icon-cache', required: false)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: (not is_ci) and gtk_update_icon_cache.found(),
  update_desktop_database: (not is_ci) and update_desktop_db.found()
)
