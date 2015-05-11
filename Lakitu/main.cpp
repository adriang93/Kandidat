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
	coords.CalculateCoords(image, filter, interlaced);
}

// Konstruktorn �r ej tagen fr�n exempelkoden.
WebcamApp::WebcamApp() {

	// L�s v�rden fr�n en fil. Vi antar att filen �r korrekt formaterad.
	// Det som l�ses in �r filterv�rdena i de f�rsta sex talen, och sedan vilket device som 
	// skall l�sas fr�n.
	std::fstream file("values.txt");
	std::string filedevice;
	file >> filter.lowH >> filter.highH >> filter.lowS >> filter.highS
		>> filter.lowV >> filter.highV >> filedevice >> port
		>> filter.minArea >> filter.maxArea >> filter.minCircularity
		>> filter.open >> filter.close >> objSize >> horisontalFov
		>> distanceCutoff >> compassCutoff >> interlaced >> console;
	//Skapa en WebcamHandler med det device som l�sts in fr�n filen.
	try {
		int device = std::stoi(filedevice);
		captureHandler.SetDevice(device);
	}
	// Om vi inte kan konvertera till int tolkar vi v�rdet som ett filnamn
	catch (...) {
		captureHandler.SetFile(filedevice);
	}

	// Skapa en konsoll om detta valts (vilket inte g�rs som stnadard i en Windowsapp.)
	if (console) {
		if (!AllocConsole()) {

			// FAIL �r ett makro fr�n OculusRiftInAction-resurserna. 
			FAIL("Could not create console");
		}
		// �ppna konsollen med magiska v�rden som g�r att det fungerar.
		// fr�n http://stackoverflow.com/questions/9020790/using-stdin-with-an-allocconsole
		freopen("CONOUT$", "w", stdout);
		//Spara ett handle till konsollen s� att vi kan flytta mark�ren senare.
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	std::cout << std::to_string(horisontalFov);
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

	// N�r initGl k�rs har moderklassens kod redan initierat Oculus Rift. hmd pekar d�rf�r
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

	// Justeringar f�r olika programfunktioner
	cv::namedWindow("Trackbars", CV_WINDOW_AUTOSIZE);
	// Skapa skjutreglage f�r alla parameterv�rden f�r objektdetekteringen. 
	// Skapa dem med referenser till filterparametrarna s� vi inte beh�ver uppdatera
	// filtret manuellt
	cv::createTrackbar("HueLow", "Trackbars", &(filter.lowH), 179);
	cv::createTrackbar("HueHigh", "Trackbars", &(filter.highH), 179);
	cv::createTrackbar("SatLow", "Trackbars", &(filter.lowS), 255);
	cv::createTrackbar("SatHigh", "Trackbars", &(filter.highS), 255);
	cv::createTrackbar("ValLow", "Trackbars", &(filter.lowV), 255);
	cv::createTrackbar("ValHigh", "Trackbars", &(filter.highV), 255);
	cv::createTrackbar("minArea", "Trackbars", &(filter.minArea), 150);
	cv::createTrackbar("maxArea", "Trackbars", &(filter.maxArea), 2000);
	cv::createTrackbar("minCircularity", "Trackbars", &(filter.minCircularity), 100);
	cv::createTrackbar("Storlek p� opening", "Trackbars", &(filter.open), 15);
	cv::createTrackbar("Storlek p� closing", "Trackbars", &(filter.close), 15);
	// Skjutreglage f�r kompassens brytfrekvens.
	cv::createTrackbar("Brytfrekvens f�r kompass", "Trackbars", &compassCutoff, 500);
	cv::createTrackbar("Brytfrekvens f�r avst�nd", "Trackbars", &distanceCutoff, 10);
}

// Denna kod k�rs av moderklassen.
void WebcamApp::update() {
	cv::Mat captureData;
	// Efterf�ljande kod kommer vara bunden till hur ofta vi kan f� en frame fr�n videoenheten.
	compass.SetSmoothing(compassCutoff);
	if (captureHandler.GetFrame(captureData)) {
		frameDelay = captureHandler.GetDelay();

		// Om bildbehandlingen slutf�rts vill vi hantera informationen och starta en ny omg�ng med 
		// senaste frame.
		if (coords.Ready()) {
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
			Coords::Coord coord = coords.GetCoords();
			
			float L = 2 * PI * frameDelay/1000 * distanceCutoff;
			float smoothing = L/(L + 1);
			
			float ang = (coord.size / returnImage.cols)*horisontalFov*(PI/180);
			int diff = (objSize / (tan(ang))) - distance;
			distance += smoothing * diff;

			std::cout << "dist: " << std::to_string(distance) 
				<< "      " << "ang: " << std::to_string(L) 
				<< "      " << "smooth: " << std::to_string(smoothing) << "\n";
			navigator->SetCoords(coord, distance);

			if (coord.valid) {
				if (cross) {
					Coords::DrawCross(coord.x, coord.y, returnImage);
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
		if (key == GLFW_KEY_C) {
			cross = !cross;
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