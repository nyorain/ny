# config file
version = meson.project_version().split('.')

conf_data = configuration_data()
conf_data.set('vmajor', version[0])
conf_data.set('vminor', version[1])
conf_data.set('vpatch', version[2])
conf_data.set('NY_WithWayland', enable_wayland)
conf_data.set('NY_WithWinapi', enable_winapi)
conf_data.set('NY_WithX11', enable_x11)
conf_data.set('NY_WithGl', enable_gl)
conf_data.set('NY_WithEgl', enable_egl)
conf_data.set('NY_WithVulkan', enable_vulkan)
conf_data.set('NY_WithXkbcommon', dep_xkbcommon.found())
conf_data.set('NY_WithAndroid', android)

configure_file(input: 'config.hpp.in',
	output: 'config.hpp',
	install_dir: 'include/ny',
	configuration: conf_data)

# install headers
# TODO: backend headers as well?
# TODO: use install_subdir
headers = [
	'appContext.hpp',
	'backend.hpp',
	'bufferSurface.hpp',
	'cursor.hpp',
	'dataExchange.hpp',
	'event.hpp',
	'fwd.hpp',
	'image.hpp',
	'key.hpp',
	'keyboardContext.hpp',
	'mouseButton.hpp',
	'mouseContext.hpp',
	'nativeHandle.hpp',
	'ny.hpp',
	'surface.hpp',
	'windowContext.hpp',
	'windowListener.hpp',
	'windowSettings.hpp',]

headers += [
	'common/egl.hpp',
	'common/gl.hpp',
	'common/unix.hpp',
	'common/copy.hpp',
	'common/xkb.hpp']

install_headers(headers, subdir: 'ny')
