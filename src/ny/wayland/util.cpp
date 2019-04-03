// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/util.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/cursor.hpp>

#include <dlg/dlg.hpp>
#include <nytl/scope.hpp>

#include <wayland-client-protocol.h>
#include <ny/wayland/protocols/xdg-shell-unstable-v6.h>
#include <ny/wayland/protocols/xdg-shell.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <cstring>

namespace ny {
namespace wayland {
namespace {

int setCloexecOrClose(int fd) {
	long flags;
	if(fd == -1) goto err2;

	flags = fcntl(fd, F_GETFD);
	if(flags == -1) goto err1;

	if(fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) goto err1;
	return fd;

err1:
	close(fd);

err2:
	dlg_warn("fctnl failed: {}", std::strerror(errno));
	return -1;
}

int createTmpfileCloexec(char *tmpname) {
	int fd;

	#ifdef HAVE_MKOSTEMP
		fd = mkostemp(tmpname, O_CLOEXEC);
		if(fd >= 0) unlink(tmpname);

	#else
		fd = mkstemp(tmpname);
		if(fd >= 0)
		{
			fd = setCloexecOrClose(fd);
			unlink(tmpname);
		}
	#endif

	return fd;
}

int osCreateAnonymousFile(off_t size) {
	static const char template1[] = "/weston-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = (char*) malloc(strlen(path) + sizeof(template1));
	if(!name) return -1;

	strcpy(name, path);
	strcat(name, template1);

	fd = createTmpfileCloexec(name);
	free(name);

	if(fd < 0) return -1;
	if(ftruncate(fd, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

} // anonymous util namespace

//shmBuffer
ShmBuffer::ShmBuffer(WaylandAppContext& ac, nytl::Vec2ui size, unsigned int stride)
		: appContext_(&ac), size_(size), stride_(stride) {
	format_ = WL_SHM_FORMAT_ARGB8888;
	if(!stride_) stride_ = size[0] * 4;
	create();
}

ShmBuffer::~ShmBuffer() {
	destroy();
}

ShmBuffer::ShmBuffer(ShmBuffer&& other) {
	appContext_ = other.appContext_;
	shmSize_ = other.shmSize_;
	size_ = other.size_;
	stride_ = other.stride_;
	buffer_ = other.buffer_;
	pool_ = other.pool_;
	data_ = other.data_;
	format_ = other.format_;
	used_ = other.used_;

	other.appContext_ = {};
	other.shmSize_ = {};
	other.size_ = {};
	other.buffer_ = {};
	other.pool_ = {};
	other.data_ = {};
	other.format_ = {};
	other.used_ = {};

	if(buffer_) wl_buffer_set_user_data(buffer_, this);
}

ShmBuffer& ShmBuffer::operator=(ShmBuffer&& other) {
	destroy();

	appContext_ = other.appContext_;
	shmSize_ = other.shmSize_;
	size_ = other.size_;
	stride_ = other.stride_;
	buffer_ = other.buffer_;
	pool_ = other.pool_;
	data_ = other.data_;
	format_ = other.format_;
	used_ = other.used_;

	other.appContext_ = {};
	other.shmSize_ = {};
	other.size_ = {};
	other.buffer_ = {};
	other.pool_ = {};
	other.data_ = {};
	other.format_ = {};
	other.used_ = {};

	if(buffer_) wl_buffer_set_user_data(buffer_, this);

	return *this;
}

void ShmBuffer::create() {
	destroy();
	if(!size_[0] || !size_[1]) throw std::runtime_error("ny::wayland::ShmBuffer invalid size");
	if(!stride_) throw std::runtime_error("ny::wayland::ShmBuffer invalid stride");

	auto* shm = appContext_->wlShm();
	if(!shm) throw std::runtime_error("ny::wayland::ShmBuffer: appContext has no wl_shm");

	auto vecSize = stride_ * size_[1];
	shmSize_ = std::max(vecSize, shmSize_);

	auto fd = osCreateAnonymousFile(shmSize_);
	if (fd < 0) throw std::runtime_error("ny::wayland::ShmBuffer: could not create shm file");

	// the fd is not needed here anymore AFTER the pool was created
	// our access to the file is represented by the pool
	auto fdGuard = nytl::ScopeGuard([&]{ close(fd); });

	auto ptr = mmap(nullptr, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(ptr == MAP_FAILED) throw std::runtime_error("ny::wayland::ShmBuffer: could not mmap file");

	data_ = reinterpret_cast<std::byte*>(ptr);
	pool_ = wl_shm_create_pool(shm, fd, shmSize_);
	buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_[0], size_[1], stride_, format_);

	static constexpr wl_buffer_listener listener {
		memberCallback<&ShmBuffer::released>
	};

	wl_buffer_add_listener(buffer_, &listener, this);
}

void ShmBuffer::destroy() {
	if(buffer_) wl_buffer_destroy(buffer_);
	if(pool_) wl_shm_pool_destroy(pool_);
	if(data_) munmap(data_, shmSize_);
}

bool ShmBuffer::size(nytl::Vec2ui size, unsigned int stride) {
	size_ = size;
	stride_ = stride;

	if(!stride_) stride_ = size[0] * 4;
	unsigned int vecSize = stride_ * size_[1];

	if(!size_[0] || !size_[1]) throw std::runtime_error("ny::wayland::ShmBuffer invalid size");
	if(!stride_) throw std::runtime_error("ny::wayland::ShmBuffer invalid stride");

	if(vecSize > shmSize_) {
		create();
		return true;
	} else {
		wl_buffer_destroy(buffer_);
		buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_[0], size_[1], stride_, format_);
		return false;
	}
}

// util
const char* errorName(const wl_interface& interface, int error) {
	struct Error {
		int code;
		const char* msg;
	};

	static struct {
		const wl_interface& interface;
		std::vector<Error> errors;
	} interfaces[] = {
		// core protocol
		{ wl_display_interface, {
			{ WL_DISPLAY_ERROR_INVALID_OBJECT, "WL_DISPLAY_ERROR_INVALID_OBJECT" },
			{ WL_DISPLAY_ERROR_INVALID_METHOD, "WL_DISPLAY_ERROR_INVALID_METHOD" },
			{ WL_DISPLAY_ERROR_NO_MEMORY, "WL_DISPLAY_ERROR_NO_MEMORY" } }
		},
		{ wl_shm_interface, {
			{ WL_SHM_ERROR_INVALID_FORMAT, "WL_SHM_ERROR_INVALID_FORMAT" },
			{ WL_SHM_ERROR_INVALID_STRIDE, "WL_SHM_ERROR_INVALID_STRIDE" },
			{ WL_SHM_ERROR_INVALID_FD, "WL_SHM_ERROR_INVALID_FD" } }
		},
		{ wl_data_offer_interface, {
			{ WL_DATA_OFFER_ERROR_INVALID_FINISH, "WL_DATA_OFFER_ERROR_INVALID_FINISH" },
			{ WL_DATA_OFFER_ERROR_INVALID_ACTION_MASK, "WL_DATA_OFFER_ERROR_INVALID_ACTION_MASK" },
			{ WL_DATA_OFFER_ERROR_INVALID_ACTION, "WL_DATA_OFFER_ERROR_INVALID_ACTION" },
			{ WL_DATA_OFFER_ERROR_INVALID_OFFER , "WL_DATA_OFFER_ERROR_INVALID_OFFER" } }
		},
		{ wl_data_source_interface, {
			{ WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK, "WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK" },
			{ WL_DATA_SOURCE_ERROR_INVALID_SOURCE, "WL_DATA_SOURCE_ERROR_INVALID_SOURCE" } }
		},
		{ wl_data_device_interface, {
			{ WL_DATA_DEVICE_ERROR_ROLE, "WL_DATA_DEVICE_ERROR_ROLE" } },
		},
		{ wl_shell_interface, {
			{ WL_SHELL_ERROR_ROLE, "WL_SHELL_ERROR_ROLE" } },
		},
		{ wl_surface_interface, {
			{ WL_SURFACE_ERROR_INVALID_SCALE, "WL_SURFACE_ERROR_INVALID_SCALE" },
			{ WL_SURFACE_ERROR_INVALID_TRANSFORM, "WL_SURFACE_ERROR_INVALID_TRANSFORM" } },
		},
		{ wl_shell_interface, {
			{ WL_POINTER_ERROR_ROLE, "WL_POINTER_ERROR_ROLE" } },
		},
		{ wl_subcompositor_interface, {
			{ WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE, "WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE" } },
		},
		{ wl_subsurface_interface, {
			{ WL_SUBSURFACE_ERROR_BAD_SURFACE, "WL_SUBSURFACE_ERROR_BAD_SURFACE" } },
		},

		// xdg
		// shell v6
		{ zxdg_shell_v6_interface, {
			{ ZXDG_SHELL_V6_ERROR_ROLE, "ZXDG_SHELL_V6_ERROR_ROLE" },
			{ ZXDG_SHELL_V6_ERROR_DEFUNCT_SURFACES, "ZXDG_SHELL_V6_ERROR_DEFUNCT_SURFACES" },
			{ ZXDG_SHELL_V6_ERROR_NOT_THE_TOPMOST_POPUP,
				"ZXDG_SHELL_V6_ERROR_NOT_THE_TOPMOST_POPUP" },
			{ ZXDG_SHELL_V6_ERROR_INVALID_POPUP_PARENT,
				"ZXDG_SHELL_V6_ERROR_INVALID_POPUP_PARENT" },
			{ ZXDG_SHELL_V6_ERROR_INVALID_SURFACE_STATE,
				"ZXDG_SHELL_V6_ERROR_INVALID_SURFACE_STATE" },
			{ ZXDG_SHELL_V6_ERROR_INVALID_POSITIONER, "ZXDG_SHELL_V6_ERROR_INVALID_POSITIONER" } },
		},
		{ zxdg_positioner_v6_interface, {
			{ ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT, "ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT"} }
		},
		{ zxdg_surface_v6_interface, {
			{ ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED, "ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED" },
			{ ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED,
				"ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED" },
			{ ZXDG_SURFACE_V6_ERROR_UNCONFIGURED_BUFFER,
				"ZXDG_SURFACE_V6_ERROR_UNCONFIGURED_BUFFER" } }
		},
		{ zxdg_popup_v6_interface, {
			{ ZXDG_POPUP_V6_ERROR_INVALID_GRAB, "ZXDG_POPUP_V6_ERROR_INVALID_GRAB"} }
		},
	};

	for(auto& i : interfaces) {
		if(&i.interface != &interface) {
			continue;
		}

		for(auto& e : i.errors) {
			if(e.code == error) {
				return e.msg;
			}
		}

		break;
	}

	return "<unknown interface error>";
}

} // namespace wayland

WindowEdge waylandToEdge(unsigned int wlEdge) {
	return static_cast<WindowEdge>(wlEdge);
}

unsigned int edgeToWayland(WindowEdge edge) {
	return static_cast<unsigned int>(edge);
}

constexpr struct FormatConversion {
	ImageFormat imageFormat;
	unsigned int shmFormat;
} formatConversions[] {
	{ImageFormat::argb8888, WL_SHM_FORMAT_ARGB8888},
	{ImageFormat::argb8888, WL_SHM_FORMAT_XRGB8888},
	{ImageFormat::rgba8888, WL_SHM_FORMAT_RGBA8888},
	{ImageFormat::bgra8888, WL_SHM_FORMAT_BGRA8888},
	{ImageFormat::abgr8888, WL_SHM_FORMAT_ABGR8888},
	{ImageFormat::bgr888, WL_SHM_FORMAT_BGR888},
	{ImageFormat::rgb888, WL_SHM_FORMAT_RGB888},
};

int imageFormatToWayland(const ImageFormat& format) {
	for(auto& fc : formatConversions) {
		if(fc.imageFormat == format) {
			return fc.shmFormat;
		}
	}

	return -1;
}

ImageFormat waylandToImageFormat(unsigned int shmFormat) {
	for(auto& fc : formatConversions) {
		if(fc.shmFormat == shmFormat) {
			return fc.imageFormat;
		}
	}

	return ImageFormat::none;
}

unsigned int stateToWayland(ToplevelState state) {
	switch(state) {
		case ToplevelState::maximized: return 1;
		case ToplevelState::fullscreen: return 2;
		default: return 0;
	}
}

ToplevelState waylandToState(unsigned int wlState) {
	switch(wlState) {
		case 1: return ToplevelState::maximized;
		case 2: return ToplevelState::fullscreen;
		default: return ToplevelState::unknown;
	}
}

DndAction waylandToDndAction(unsigned int wlAction) {
	switch(wlAction) {
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY: return DndAction::copy;
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE: return DndAction::move;
		default: return DndAction::none;
	}
}

nytl::Flags<DndAction> waylandToDndActions(unsigned int wlAction) {
	nytl::Flags<DndAction> ret {};
	if(wlAction & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
		ret |= DndAction::copy;
	}

	if(wlAction & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) {
		ret |= DndAction::move;
	}

	return ret;
}

unsigned int dndActionToWayland(DndAction action) {
	switch(action) {
		case DndAction::copy: return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
		case DndAction::move: return WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
		default: return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
	}
}

unsigned int dndActionsToWayland(nytl::Flags<DndAction> wlAction) {
	unsigned int ret  = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
	if(wlAction & DndAction::copy) {
		ret |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
	}

	if(wlAction & DndAction::move) {
		ret |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
	}

	return ret;
}

} // namespace ny
