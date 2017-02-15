#include "openGLApp.h"

int main(int argc, char *argv[]) {
	OpenGLApp app;
	app.initOpenGL(argc, argv);

	app.runOpenGL();

	return 0;
}