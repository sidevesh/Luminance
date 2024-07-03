schemadir = join_paths(get_option('datadir'), 'glib-2.0/schemas')
install_data([
	'com.sidevesh.Luminance.gschema.xml',
], install_dir: schemadir)

icondir = join_paths(get_option('datadir'), 'icons/hicolor')

install_data('com.sidevesh.Luminance.svg',
	install_dir: join_paths(icondir, 'scalable/apps')
)

install_data('com.sidevesh.Luminance-symbolic.svg',
	install_dir: join_paths(icondir, 'symbolic/apps')
)
desktop = 'ccom.sidevesh.Luminance.desktop'
install_data('com.sidevesh.Luminance.desktop',
	install_dir: join_paths(get_option('datadir'), 'applications')
)

install_data('com.sidevesh.Luminance.desktop',
	install_dir: join_paths(get_option('datadir'), 'applications')
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('Validate desktop file', desktop_file_validate, args: [desktop])
endif

glib_compile_schemas = find_program('glib-compile-schemas', required: false)
if glib_compile_schemas.found()
  test('Validate schema file', glib_compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif
