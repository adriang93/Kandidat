/* 

Huvudmodul f�r programmet. Mycket av koden kommer fr�n exempel 13.2 fr�n OculusRiftInAction, 
anpassad f�r v�rt projekt.

TODO: Licens f�r ovanst�ende.

*/ 

#include "stdafx.h"
#include "main.h"

// Wrapperkod f�r att kunna k�ra CalculateCoords i en separat tr�d.
// B�r g� att k�ra kod fr�n externa klasser i egen tr�d utan detta. 
// TODO: Hitta s�tt att skippa denna kodsnutt.
void WebcamApp::calcCoordsCall(cv::Mat& image) {
	coords.CalculateCoords(image, minArea, maxArea, minCircularity, open, close);
}

// Konstruktorn �r ej tagen fr�n exempelkoden.
WebcamApp::WebcamApp() : glewExperimental(GL_TRUE) {
	// L�s v�rden fr�n en fil. Vi antar att filen �r korrekt formaterad.
	// Det som l�ses in �r filterv�rdena i de f�rsta sex talen, och sedan vilket device som 
	// skall l�sas fr�n.
	std::fstream file("values.txt");
	std::string filedevice;
	file >> filter.lowH >> filter.highH >> filter.lowS >> filter.highS 
		>> filter.lowV >> filter.highV >> filedevice >> port 
		>> minArea >> maxArea >> minCircularity;
	//Skapa en WebcamHandler med det device som l�sts in fr�n filen.
	try {
		int device = std::stoi(filedevice);
		captureHandler.SetDevice(device);
	}
	catch (...) {
		captureHandler.SetFile(filedevice);
	}
	
	// S�tt filtret i instansen av Coords till det vi l�st in fr�n filen.
	coords.SetHSV(filter); 

	// Skapa en konsoll (vilket inte g�rs som stnadard i en Windowsapp.)
//	if (!AllocConsole()) {

		// FAIL �r ett makro fr�n OculusRiftInAction-resurserna. 
//		FAIL("Could not create console");
//	}
	// �ppna konsollen med magiska v�rden som g�r att det fungerar.
//	freopen("CONOUT$", "w", stdout);
	
	//Spara ett handle till konsollen s� att vi kan flytta mark�ren senare.
}

// Destruktor. Avslutar startade tr�dar.
WebcamApp::~WebcamApp() {
	delete navigator;

	// Stoppa bildextraheringen. 
	captureHandler.StopCapture();

	// L�t ber�kningne slutf�ras.
	calcThread.join();
}

// Initiera OpenGL-systemet. Det mesta av jobbet g�rs i moderklassen. 
void WebcamApp::initGl() {

	//Avkommentera f�r att visa en fin bild p� Lakitu!
	//cv::imshow("Lakitu", cv::imread("Resources/lakitu.png", CV_LOAD_IMAGE_COLOR));

	RiftApp::initGl();
	
	// N�r initGl k�rs har moderklassens kod redna initierat Oculus Rift. hmd pekar d�rf�r
	// p� ett OR-objekt nu. S�tt v�rdet i kompassmodulen till detta, och starta kompassber�kningen.
	compass.SetHMD(&hmd);
	compass.Start();
	navigator = new NavigatorComm(compass, port);

	// Kod fr�n exempel 13.2. Sj�lvf�rklarande tycker jag.
	using namespace oglplus;
	texture = TexturePtr(new Texture());
	Context::Bound(TextureTarget::_2D, *texture)
		.MagFilter(TextureMagFilter::Linear)
		.MinFilter(TextureMinFilter::Linear);
	program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
	float aspectRatio = captureHandler.StartCapture(); // <- startar bildextrahering
	videoGeometry = oria::loadPlane(program, aspectRatio);
	cv::namedWindow("Trackbars", CV_WINDOW_NORMAL);
	cv::createTrackbar("HueLow", "Trackbars", &(filter.lowH),179);
	cv::createTrackbar("HueHigh", "Trackbars", &(filter.highH), 179);
	cv::createTrackbar("SatLow", "Trackbars", &(filter.lowS), 255);
	cv::createTrackbar("SatHigh", "Trackbars", &(filter.highS), 255);
	cv::createTrackbar("ValLow", "Trackbars", &(filter.lowV), 255);
	cv::createTrackbar("ValHigh", "Trackbars", &(filter.highV), 255);
	cv::createTrackbar("minArea", "Trackbars", &minArea, 150);
	cv::createTrackbar("maxArea", "Trackbars", &maxArea, 2000);
	cv::createTrackbar("minCircularity", "Trackbars", &minCircularity, 100);
	cv::createTrackbar("opening", "Trackbars", &open, 15);
	cv::createTrackbar("close", "Trackbars", &close, 15);
}

// Denna kod k�rs "ofta" av moderklassen.
void WebcamApp::update() {
	cv::Mat captureData;

	// Efterf�ljande kod kommer vara bunden till hur ofta vi kan f� en frame fr�n videoenheten.
	if (captureHandler.GetFrame(captureData)) {

		// Om bildbehandlingen slutf�rts vill vi hantera informationen och starta en ny omg�ng med 
		// senaste frame.
		if (coords.Ready()) {

			coords.SetHSV(filter);

			calcThread.join();

			// Beroende p� v�rdet p� variabeln displayMode visar vi olika bilder.
			// TODO: Red ut om vi kan effektivisera genom att inte kopiera bilden p� 
			// en massa st�llen, till exempel d�r en kopia returneras av GetFilteredImage.
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
			}

			// V�nd bilden r�tt. Den extraheras n�mligen upp-och-ner.
			cv::flip(returnImage.clone(), returnImage, 0);

			// Visa den returnerade bilden.
			imshow("Image", returnImage);
			
			// Starta ny ber�kningst�d med senaste frame.
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Om Coords inte �r ready, men vi inte heller n�gonsin startat, s� starta ber�kningen.
		else if (!started) {
			started = true;
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// F�rklarar sig sj�lv!
		using namespace oglplus;
		Context::Bound(TextureTarget::_2D, *texture)
			.Image2D(0, PixelDataInternalFormat::RGBA8,
			captureData.cols, captureData.rows, 0,
			PixelDataFormat::BGR, PixelDataType::UnsignedByte,
			captureData.data);
	}
}

// Hantera tangenttryckningar.
// TODO: N�r man sl�r p� cirkeldetektering kraschar just nu koden av ok�nd anledning.
void WebcamApp::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_M) {
			displayMode = !displayMode;
		}
		else if (key == GLFW_KEY_L) {
			navigator->Land();
		}
		else if (key == GLFW_KEY_S) {
			navigator->Stop();
		}
		else if (key == GLFW_KEY_R) {
			navigator->PrintLine("RESET");
		}
	}
	RiftApp::onKey(key, scancode, action, mods);
}

// Rendera
void WebcamApp::renderScene() {

	// Helt sj�lvklart!
	using namespace oglplus;
	
	// Rensa allt som renderats hittills. Den h�r delen av kdoen f�rst�r jag! /Andr�
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