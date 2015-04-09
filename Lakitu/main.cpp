#include "stdafx.h"

#include "main.h"




void WebcamApp::calcCoordsCall(cv::Mat& image) {
	doneCalculating = false;
	coords.CalculateCoords(image);
	doneCalculating = true;
}

WebcamApp::WebcamApp() {
	std::fstream file("values.txt");
	int a, b, c, d, e, f, g;
	file >> a >> b >> c >> d >> e >> f >> g;
	//coords = Coords(a, b, c, d, e, f);

	WebcamHandler webcamHandler(g);

	if (!AllocConsole()) {
		FAIL("Could not create console");
	}
	freopen("CONOUT$", "w", stdout);
}

WebcamApp::~WebcamApp() {
	captureHandler.StopCapture();
	calcThread.join();
}

void WebcamApp::initGl() {
	//cv::imshow("Lakitu", cv::imread("Resources/lakitu.png", CV_LOAD_IMAGE_COLOR));
	compass.SetHMD(&hmd);
	compass.Start();

	RiftApp::initGl();
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
/*		if (doneCalculating) {
			cv::flip(returnImage.clone(), returnImage, 0);
			imshow("Bild", returnImage);
			calcThread.join();
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}
		else if (!started) {
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
			started = true;
		}*/
		std::cout << "Heading: " << compass.FilteredHeading() << std::endl;
		using namespace oglplus;
		Context::Bound(TextureTarget::_2D, *texture)
			.Image2D(0, PixelDataInternalFormat::RGBA8,
			captureData.cols, captureData.rows, 0,
			PixelDataFormat::BGR, PixelDataType::UnsignedByte,
			captureData.data);
	}
}
void WebcamApp::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_R) {
		//coords.toggleImageFormat();
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

/*
int main(int argc, char** argv)
{



	if (!cap.isOpened())  // if not success, exit program
	{
		std::cout << "Cannot access videostream" << std::endl;
		return 0;
	}

	while (true) {

		cv::Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			std::cout << "Cannot read a frame from video stream" << std::endl;
			break;
		}

		Coords::HSVfilter filter(0, 255, 0, 255, 0, 255);

		Coords coordsModule(filter);
		coordsModule.CalculateCoords(imgOriginal);
		if (coordsModule.ValidCoords()) {
			std::pair<int, int> coords = coordsModule.GetCoords();
			std::cout << "(" << coords.first << "," << coords.second << ")" << std::endl;
		}
		else
		{
			std::cout << "(" << -1 << "," << -1 << ")" << std::endl;
		}
		cv::imshow("Bild", coordsModule.GetFilteredImage());
	}

	return 0;
}
*/