#include <ny/ny.hpp>
#include <ny/common/gl.hpp>
#include <dlg/dlg.hpp>

#include <nytl/mat.hpp>
#include <nytl/matOps.hpp>
#include <nytl/vecOps.hpp>

// TODO: we should load the function pointers
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <fstream>

// small example for trying out legacy opengl

struct Camera {
	nytl::Vec3f pos {0.f, 0.f, 3.f};
	nytl::Vec3f dir {0.f, 0.f, -1.f};
	nytl::Vec3f up {0.f, 1.f, 0.f};
	float yaw {0.f};
	float pitch {0.f};
};

nytl::Mat4f viewMatrix(Camera& c);
void rotateView(Camera& c, float dyaw, float dpitch);
std::string readFile(const char* path);

void checkShaderErrors(unsigned shader, const char* name);
void checkProgramErrors(unsigned program);

// Window
class Window : public ny::WindowListener {
public:
	Window(ny::AppContext& ac);
	~Window();

	void draw(const ny::DrawEvent& ev) override;
	void close(const ny::CloseEvent&) override { closed_ = true; }
	void mouseMove(const ny::MouseMoveEvent& ev) override;
	void mouseButton(const ny::MouseButtonEvent& ev) override;
	void key(const ny::KeyEvent& ev) override;
	void resize(const ny::SizeEvent& ev) override;

	bool closed() const { return closed_; }

protected:
	std::unique_ptr<ny::WindowContext> wc_;
	std::unique_ptr<ny::GlContext> glContext_;
	ny::AppContext* ac_;
	ny::GlSurface* surface_;
	bool closed_ {};
	nytl::Vec2ui size_ {800, 500};
	unsigned shader_ {};

	bool rotateView_ {};
	Camera camera_ {};
};

Window::Window(ny::AppContext& ac) : ac_(&ac) {
	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::gl;
	settings.gl.storeSurface = &surface_;
	settings.listener = this;
	settings.size = size_;
	wc_ = ac.createWindowContext(settings);

	ny::GlContextSettings glSettings;
	glSettings.compatibility = true;
	glContext_ = ac_->glSetup()->createContext(*surface_, glSettings);

	// will stay current for the rest of the program
	glContext_->makeCurrent(*surface_);
	dlg_info("gl version: {}", glGetString(GL_VERSION));

	int num;
	glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &num);
	for(auto i = 0; i < num; ++i) {
		auto v = glGetStringi(GL_SHADING_LANGUAGE_VERSION, i);
		dlg_info("\tglsl supported: {}", v);
	}

	// setup shader
	auto vertex = glCreateShader(GL_VERTEX_SHADER);
	auto vertSource = readFile("../src/examples/data/playground.vert");
	auto data = vertSource.data();
	GLint size = vertSource.size();
	glShaderSource(vertex, 1, &data, &size);
	glCompileShader(vertex);
	checkShaderErrors(vertex, "vertex");

	auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
	auto fragSource = readFile("../src/examples/data/playground.frag");
	data = fragSource.data();
	size = fragSource.size();
	glShaderSource(fragment, 1, &data, &size);
	glCompileShader(fragment);
	checkShaderErrors(fragment, "fragment");

	shader_ = glCreateProgram();
	glAttachShader(shader_, vertex);
	glAttachShader(shader_, fragment);
	glLinkProgram(shader_);
	checkProgramErrors(shader_);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	// setup general settings
	glClearColor(0.8, 0.9, 0.3, 1.0);
	glColor3f(1,1,1);

	glEnable(GL_DEPTH_TEST);
	glUseProgram(shader_);
}

Window::~Window() {
	if(shader_) {
		glDeleteProgram(shader_);
	}
}

void Window::resize(const ny::SizeEvent& ev) {
	size_ = ev.size;
}

void Window::mouseButton(const ny::MouseButtonEvent& ev) {
	if(ev.button == ny::MouseButton::left) {
		rotateView_ = ev.pressed;
	}
}

void Window::mouseMove(const ny::MouseMoveEvent& ev) {
	if(rotateView_) {
		rotateView(camera_, 0.005 * ev.delta.x, 0.005 * ev.delta.y);
		wc_->refresh();
	}
}

void Window::key(const ny::KeyEvent&) {
	wc_->refresh();
}

void Window::draw(const ny::DrawEvent&) {
	// input
	bool input = false;
	auto kc = ac_->keyboardContext();
	auto fac = 0.01; // TODO: somehow use delta?

	auto yUp = nytl::Vec3f {0.f, 1.f, 0.f};
	auto right = nytl::normalized(nytl::cross(camera_.dir, yUp));
	auto up = nytl::normalized(nytl::cross(camera_.dir, right));
	if(kc->pressed(ny::Keycode::d)) { // right
		camera_.pos += fac * right;
		input = true;
	}
	if(kc->pressed(ny::Keycode::a)) { // left
		camera_.pos += -fac * right;
		input = true;
	}
	if(kc->pressed(ny::Keycode::w)) {
		camera_.pos += fac * camera_.dir;
		input = true;
	}
	if(kc->pressed(ny::Keycode::s)) {
		camera_.pos += -fac * camera_.dir;
		input = true;
	}
	if(kc->pressed(ny::Keycode::q)) { // up
		camera_.pos += -fac * up;
		input = true;
	}
	if(kc->pressed(ny::Keycode::e)) { // down
		camera_.pos += fac * up;
		input = true;
	}

	// drawing
	glViewport(0, 0, size_.x, size_.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	auto aspect = float(size_.x) / size_.y;
	glFrustum(-aspect * 0.1f, aspect * 0.1f, -0.1f, 0.1f, 0.1, 10.f);

	glMatrixMode(GL_MODELVIEW);
	auto mat = viewMatrix(camera_);
	glLoadTransposeMatrixf(&mat[0][0]);

	// draw box
	static const GLfloat vertices[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};

	auto size = sizeof(vertices) / sizeof(vertices[0]);
	glBegin(GL_TRIANGLES); {
		for(auto i = 0u; i < size; i += 3) {
			glVertex3fv(&vertices[i]);
		}
	} glEnd();

	wc_->frameCallback();
	surface_->apply();

	// schedule next frame
	if(input) {
		wc_->refresh();
	}
}

// camera implementation
template<typename P>
nytl::SquareMat<4, P> lookAtRH(const nytl::Vec3<P>& eye,
		const nytl::Vec3<P>& center, const nytl::Vec3<P>& up) {

	const auto z = normalized(center - eye);
	const auto x = normalized(cross(z, up));
	const auto y = cross(x, z);

	auto ret = nytl::identity<4, P>();

	ret[0] = nytl::Vec4f(x);
	ret[1] = nytl::Vec4f(y);
	ret[2] = -nytl::Vec4f(z);

	ret[0][3] = -dot(x, eye);
	ret[1][3] = -dot(y, eye);
	ret[2][3] = dot(z, eye);

	return ret;
}

nytl::Mat4f viewMatrix(Camera& c) {
	return lookAtRH(c.pos, c.pos + c.dir, c.up);
}

void rotateView(Camera& c, float dyaw, float dpitch) {
	using nytl::constants::pi;
	c.yaw += dyaw;
	c.pitch += dpitch;
	c.pitch = std::clamp<float>(c.pitch, -pi / 2 + 0.1, pi / 2 - 0.1);

	c.dir.x = std::sin(c.yaw) * std::cos(c.pitch);
	c.dir.y = -std::sin(c.pitch);
	c.dir.z = -std::cos(c.yaw) * std::cos(c.pitch);
	nytl::normalize(c.dir);
}

// util
void checkShaderErrors(unsigned shader, const char* name) {
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		char log[2048] = {};
		glGetShaderInfoLog(shader, 2048, NULL, log);
		dlg_error("failed to compile shader {}: {}", name, log);
		throw std::runtime_error("shader error");
	}
}

void checkProgramErrors(unsigned program) {
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		char log[2048] = {};
		glGetShaderInfoLog(program, 2048, NULL, log);
		dlg_error("failed to link program: {}", log);
		throw std::runtime_error("shader error");
	}
}

std::string readFile(const char* path) {
	std::ifstream t(path);
	t.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	return {(std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>()};
}

// main
int main() {
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();
	if(!ac->glSetup()) {
		dlg_fatal("Backends doesn't have gl setup");
		return -1;
	}
	Window window(*ac);

	while(!window.closed()) {
		ac->waitEvents();
	}

	dlg_trace("window closed, exiting");
}

