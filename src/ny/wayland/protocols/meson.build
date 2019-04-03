wayland_scanner_dep = dependency('wayland-scanner', required: false,
	native: true)
if wayland_scanner_dep.found()
	wayland_scanner = find_program(
		wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner'),
		native: true)
else
	wayland_scanner = find_program('wayland-scanner', native: true)
endif

if not wayland_scanner.found()
	error('wayland scanner not found! Needed for protocols')
endif

proto_dir = dep_wl_protos.get_pkgconfig_variable('pkgdatadir')
protocols = [
	[proto_dir, 'unstable/xdg-decoration/xdg-decoration-unstable-v1.xml'],
	[proto_dir, 'unstable/xdg-shell/xdg-shell-unstable-v6.xml'],
	[proto_dir, 'stable/xdg-shell/xdg-shell.xml'],
]

wl_protos_src = []
wl_protos_headers = []

foreach p : protocols
	xml = join_paths(p)
	wl_protos_src += custom_target(
		xml.underscorify() + '_server_c',
		input: xml,
		output: '@BASENAME@.c',
		command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
	)
	wl_protos_headers += custom_target(
		xml.underscorify() + '_server_h',
		input: xml,
		output: '@BASENAME@.h',
		command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
	)
endforeach
