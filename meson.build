project(
	'Luminance',
	'c',
	version: '1.1.0',
	default_options : ['warning_level=3']
)

project_source_files = [
    'src/osd/osd.c',
    'src/osd/providers/experimental/gnome.c',
    'src/states/laptop_lid.c',
    'src/states/should_hide_internal_if_lid_closed.c',
    'src/states/displays.c',
    'src/ui/components/display_icon.c',
    'src/ui/components/display_label.c',
    'src/ui/components/link_brightness_check_button.c',
    'src/ui/components/display_brightness_scale.c',
    'src/ui/components/separator.c',
    'src/ui/screens/show_displays.c',
    'src/ui/screens/refreshing_displays.c',
    'src/ui/screens/no_displays.c',
    'src/ui/window.c',
    'src/main.c'
]

project_include_directories = include_directories('include')

libddcbc_proj = subproject('ddcbc-api')
libddcbc_dep = libddcbc_proj.get_variable('libddcbc_dep')

project_dependencies = [
	dependency('gtk4'),
	dependency('libadwaita-1'),
	dependency('threads'),
	libddcbc_dep
]

schemas_dir = get_option('prefix') / get_option('datadir') / 'glib-2.0' / 'schemas'
settings_schemas = [ 'data/com.sidevesh.Luminance.gschema.xml' ]

install_data(settings_schemas, install_dir: schemas_dir)

# Compile the schemas in the current directory; this is only
# useful for testing
gnome = import('gnome')
gnome.compile_schemas(depend_files: files(settings_schemas))


executable(meson.project_name(), project_source_files, dependencies: project_dependencies, include_directories: project_include_directories, install: true)

meson.add_install_script('glib-compile-schemas', schemas_dir)
