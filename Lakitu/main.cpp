#include "stdafx.h"

#include "main.h"




void WebcamApp::calcCoordsCall(cv::Mat& image) {
	coords.CalculateCoords(image);
}

void WebcamApp::SetConsoleRow(int row) {
	COORD pos;
	pos.X = 0;
	pos.Y = row;
	SetConsoleCursorPosition(consoleHandle, pos);
}

WebcamApp::WebcamApp() {
	std::fstream file("values.txt");
	int a, b, c, d, e, f, g;
	file >> a >> b >> c >> d >> e >> f >> g;
	Coords::HSVfilter filter(a, b, c, d, e, f);
	WebcamHandler webcamHandler(g);
	coords.SetHSV(filter); 
	coords.SetMode(coordsMode);

	if (!AllocConsole()) {
		FAIL("Could not create console");
	}
	freopen("CONOUT$", "w", stdout);
	consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

WebcamApp::~WebcamApp() {
	captureHandler.StopCapture();
	calcThread.join();
}

void WebcamApp::initGl() {
	//cv::imshow("Lakitu", cv::imread("Resources/lakitu.png", CV_LOAD_IMAGE_COLOR));

	RiftApp::initGl();
	compass.SetHMD(&hmd);
	compass.Start();

	using namespace oglplus;
	texture = TexturePtr(new Texture());
	Context::Bound(TextureTarget::_2D, *texture)
		.MagFilter(TextureMagFilter::Linear)
		.MinFilter(TextureMinFilter::Linear);
	program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
	float aspectRatio = captureHandler.StartCapture();
	videoGeometry = oria::loadPlane(program, aspectRatio);
}

void WebcamApp::update() {
	cv::Mat captureData;
	if (captureHandler.GetFrame(captureData)) {
		if (coords.Ready()) {
			if (!displayMode) {
				returnImage = captureData;
			}
			else if (displayMode == 1) {
				returnImage = coords.GetFilteredImage();
			}
			else {
				returnImage = coords.GetCircledImage();
			}
			cv::flip(returnImage, returnImage, 0);

			SetConsoleRow(rows::posRow);
			std::pair<int, int> pos = coords.GetCoords();
			std::cout << "pos: " << pos.first << ", " << pos.second;
			SetConsoleRow(rows::validRow);
			std::cout << "valid: " << coords.ValidCoords();
			SetConsoleRow(rows::corrRow);
			std::cout << "corr: " << coords.GetCorrellation();

			Coords::DrawCross(pos.first, pos.second, returnImage);
			cv::putText(returnImage, "Test", cv::Point(0, 0), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar::all(255));
			imshow("Bild", returnImage);
			calcThread.join();
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}
		else if (!started) {
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
			started = true;
		}

		SetConsoleRow(rows::headingOVRRow);
		std::cout << "Heading: " << compass.FilteredHeading();
		using namespace oglplus;
		Context::Bound(TextureTarget::_2D, *texture)
			.Image2D(0, PixelDataInternalFormat::RGBA8,
			captureData.cols, captureData.rows, 0,
			PixelDataFormat::BGR, PixelDataType::UnsignedByte,
			captureData.data);
	}
}
void WebcamApp::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_C) {
			coordsMode ^= Coords::COORDS_CIRCLE;
		}
		else if (key == GLFW_KEY_F) {
			coordsMode ^= Coords::COORDS_FILTER;
		}
		else if (key == GLFW_KEY_M) {
			displayMode++;
			if (displayMode == 3) {
				displayMode = 0;
			}
		}
		coords.SetMode(coordsMode);
		SetConsoleRow(rows::modeRow);
		std::cout << "Mode: " << displayMode;
		SetConsoleRow(rows::coordsModeRow);
		std::cout << "coordsMode: " << coordsMode;
	}
	RiftApp::onKey(key, scancode, action, mods);
}

void WebcamApp::renderScene() {
	using namespace oglplus;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	MatrixStack & mv = Stacks::modelview();
	mv.withPush([&] {
		mv.identity();
		// Uncomment to position the frame always in front of you
		//mv.preMultiply(headPose);  
		mv.translate(glm::vec3(0, 0, -1));
		texture->Bind(TextureTarget::_2D);
		oria::renderGeometry(videoGeometry, program);
		oglplus::DefaultTexture().Bind(TextureTarget::_2D);
	});
}

RUN_OVR_APP(WebcamApp);