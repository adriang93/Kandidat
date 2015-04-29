/* 

Huvudmodul för programmet. Mycket av koden kommer från exempel 13.2 från OculusRiftInAction, 
anpassad för vårt projekt.

TODO: Licens för ovanstående.

*/ 

#include "stdafx.h"
#include "main.h"

// Wrapperkod för att kunna köra CalculateCoords i en separat tråd.
// Bör gå att köra kod från externa klasser i egen tråd utan detta. 
// TODO: Hitta sätt att skippa denna kodsnutt.
void WebcamApp::calcCoordsCall(cv::Mat& image) {
	coords.CalculateCoords(image, minArea, maxArea, minCircularity);
}

// Konstruktorn är ej tagen från exempelkoden.
WebcamApp::WebcamApp() {
	// Läs värden från en fil. Vi antar att filen är korrekt formaterad.
	// Det som läses in är filtervärdena i de första sex talen, och sedan vilket device som 
	// skall läsas från.
	std::fstream file("values.txt");
	std::string filedevice;
	file >> filter.lowH >> filter.highH >> filter.lowS >> filter.highS 
		>> filter.lowV >> filter.highV >> filedevice >> port 
		>> minArea >> maxArea >> minCircularity;
	//Skapa en WebcamHandler med det device som lästs in från filen.
	try {
		int device = std::stoi(filedevice);
		captureHandler.SetDevice(device);
	}
	catch (...) {
		captureHandler.SetFile(filedevice);
	}
	
	// Sätt filtret i instansen av Coords till det vi läst in från filen.
	coords.SetHSV(filter); 

	// Skapa en konsoll (vilket inte görs som stnadard i en Windowsapp.)
//	if (!AllocConsole()) {

		// FAIL är ett makro från OculusRiftInAction-resurserna. 
//		FAIL("Could not create console");
//	}
	// Öppna konsollen med magiska värden som gör att det fungerar.
//	freopen("CONOUT$", "w", stdout);
	
	//Spara ett handle till konsollen så att vi kan flytta markören senare.
}

// Destruktor. Avslutar startade trådar.
WebcamApp::~WebcamApp() {
	delete navigator;

	// Stoppa bildextraheringen. 
	captureHandler.StopCapture();

	// Låt beräkningne slutföras.
	calcThread.join();
}

// Initiera OpenGL-systemet. Det mesta av jobbet görs i moderklassen. 
void WebcamApp::initGl() {

	//Avkommentera för att visa en fin bild på Lakitu!
	//cv::imshow("Lakitu", cv::imread("Resources/lakitu.png", CV_LOAD_IMAGE_COLOR));

	RiftApp::initGl();
	
	// När initGl körs har moderklassens kod redna initierat Oculus Rift. hmd pekar därför
	// på ett OR-objekt nu. Sätt värdet i kompassmodulen till detta, och starta kompassberäkningen.
	compass.SetHMD(&hmd);
	compass.Start();
	navigator = new NavigatorComm(compass, port);

	// Kod från exempel 13.2. Självförklarande tycker jag.
	using namespace oglplus;
	texture = TexturePtr(new Texture());
	Context::Bound(TextureTarget::_2D, *texture)
		.MagFilter(TextureMagFilter::Linear)
		.MinFilter(TextureMinFilter::Linear);
	program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
	float aspectRatio = captureHandler.StartCapture(); // <- startar bildextrahering
	videoGeometry = oria::loadPlane(program, aspectRatio);
	cv::namedWindow("Trackbars");
	cv::createTrackbar("HueLow", "Trackbars", &(filter.lowH),179);
	cv::createTrackbar("HueHigh", "Trackbars", &(filter.highH), 179);
	cv::createTrackbar("SatLow", "Trackbars", &(filter.lowS), 255);
	cv::createTrackbar("SatHigh", "Trackbars", &(filter.highS), 255);
	cv::createTrackbar("ValLow", "Trackbars", &(filter.lowV), 255);
	cv::createTrackbar("ValHigh", "Trackbars", &(filter.highV), 255);
	cv::createTrackbar("minArea", "Trackbars", &minArea, 150);
	cv::createTrackbar("maxArea", "Trackbars", &maxArea, 2000);
	cv::createTrackbar("minCircularity", "Trackbars", &minCircularity, 100);
}

// Denna kod körs "ofta" av moderklassen.
void WebcamApp::update() {
	cv::Mat captureData;

	// Efterföljande kod kommer vara bunden till hur ofta vi kan få en frame från videoenheten.
	if (captureHandler.GetFrame(captureData)) {

		// Om bildbehandlingen slutförts vill vi hantera informationen och starta en ny omgång med 
		// senaste frame.
		if (coords.Ready()) {

			coords.SetHSV(filter);

			calcThread.join();

			// Beroende på värdet på variabeln displayMode visar vi olika bilder.
			// TODO: Red ut om vi kan effektivisera genom att inte kopiera bilden på 
			// en massa ställen, till exempel där en kopia returneras av GetFilteredImage.
			if (displayMode == 0) {
				returnImage = captureData.clone();
			}
			else if (displayMode == 1) {
				returnImage = coords.GetFilteredImage();
			}
			std::pair<int, int> pos = coords.GetCoords();

			navigator->SetCoords(pos, coords.ValidCoords());

			if (coords.ValidCoords()) {
				if (cross) {
					Coords::DrawCross(pos.first, pos.second, returnImage);
				}
				missedFramesCount = 0;
			}
			else {
				missedFramesCount++;
				if (missedFramesCount >= 10) {
					navigator->Land(); // tappar bort användaren i en halv sekund
					//TODO: Indikera detta för användaren och gör fler saker
				}
			}

			// Vänd bilden rätt. Den extraheras nämligen upp-och-ner.
			cv::flip(returnImage.clone(), returnImage, 0);

			// Visa den returnerade bilden.
			imshow("Image", returnImage);
			
			// Starta ny beräkningståd med senaste frame.
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Om Coords inte är ready, men vi inte heller någonsin startat, så starta beräkningen.
		else if (!started) {
			started = true;
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Förklarar sig själv!
		using namespace oglplus;
		Context::Bound(TextureTarget::_2D, *texture)
			.Image2D(0, PixelDataInternalFormat::RGBA8,
			captureData.cols, captureData.rows, 0,
			PixelDataFormat::BGR, PixelDataType::UnsignedByte,
			captureData.data);
	}
}

// Hantera tangenttryckningar.
// TODO: När man slår på cirkeldetektering kraschar just nu koden av okänd anledning.
void WebcamApp::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_M) {
			displayMode = !displayMode;
		}
		else if (key == GLFW_KEY_ESCAPE) {
			navigator->Land();
		}
		else if (key == GLFW_KEY_S) {
			navigator->Stop();
		}
		else if (key == GLFW_KEY_8) {
			navigator->PrintLine("Hejsanhoppsan!");
		}
	}
	RiftApp::onKey(key, scancode, action, mods);
}

// Rendera
void WebcamApp::renderScene() {

	// Helt självklart!
	using namespace oglplus;
	
	// Rensa allt som renderats hittills. Den här delen av kdoen förstår jag! /André
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