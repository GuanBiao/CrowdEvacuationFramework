#include "openGLApp.h"
#include "testApp.h"

int main(int argc, char *argv[]) {
	OpenGLApp app;
	app.initOpenGL(argc, argv);

	app.runOpenGL();

	//TestApp app;

	//app.runTest();

	return 0;
}