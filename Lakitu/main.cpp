/*

Huvudmodul f�r programmet. Mycket av koden kommer fr�n exempel 13.2 fr�n OculusRiftInAction,
anpassad f�r v�rt projekt.

https://github.com/OculusRiftInAction/OculusRiftInAction/
Licensierad under Apache 2.0

All kod utom det som markerats skriven av Andr� Wallstr�m.

*/

#include "stdafx.h"
#include "main.h"

// Wrapperkod f�r att kunna k�ra CalculateCoords i en separat tr�d.
// B�r g� att k�ra kod fr�n externa klasser i egen tr�d utan detta. 
// TODO: Hitta s�tt att skippa denna kodsnutt.
void WebcamApp::calcCoordsCall(cv::Mat& image) {
	coords.CalculateCoords(image, filter, interlaced, displayMode);
}

// Konstruktorn �r ej tagen fr�n exempelkoden.
WebcamApp::WebcamApp() {

	// L�s v�rden fr�n en fil. Vi antar att filen �r korrekt formaterad.
	// I produktionskod hade detta givetvis varit skrivet med korrekt typechecking.
	// Detta har dock inte prioriterats i projektet.
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

	// Skapa en konsoll (vilket inte g�rs som stnadard i en Windowsapp) om detta valts via values.txt
	if (console) {
		if (!AllocConsole()) {

			// FAIL �r ett makro fr�n OculusRiftInAction-resurserna som l�ter
			// programmet krascha efter att feltexten visats
			FAIL("Could not create console");
		}

		// �ppna konsollen med magiska v�rden som g�r att det fungerar.
		// fr�n http://stackoverflow.com/questions/9020790/using-stdin-with-an-allocconsole
		freopen("CONOUT$", "w", stdout);
		// Spara ett handle till konsollen s� att vi kan modifiera den senare.
		// Anv�nds just nu inte alls.
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

// Destruktor. Avslutar startade tr�dar. Ej tagen fr�n exempelkoden.
WebcamApp::~WebcamApp() {
	delete navigator;

	// Stoppa bildextraheringen. 
	captureHandler.StopCapture();

	// L�t ber�kningne slutf�ras.
	calcThread.join();
}

// Initiera OpenGL-systemet. Det mesta av jobbet g�rs i moderklassen. 
void WebcamApp::initGl() {

	// Fr�n exempelkoden
	RiftApp::initGl();

	// N�r initGl k�rs har moderklassens kod redan initierat Oculus Rift. hmd pekar d�rf�r
	// p� ett OR-objekt nu. S�tt v�rdet i kompassmodulen till detta, och starta kompassber�kningen.
	compass.SetHMD(&hmd);
	compass.Start();
	navigator = new NavigatorComm(compass, port);

	// Kod fr�n exempel 13.2, ej modifierad av projektet
	using namespace oglplus;
	texture = TexturePtr(new Texture());
	Context::Bound(TextureTarget::_2D, *texture)
		.MagFilter(TextureMagFilter::Linear)
		.MinFilter(TextureMinFilter::Linear);
	program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
	float aspectRatio = captureHandler.StartCapture(); // <- startar bildextrahering
	videoGeometry = oria::loadPlane(program, aspectRatio);

	// Justeringar f�r olika programfunktioner. Skapat av projektet.
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

// Denna kod k�rs av moderklassen. Allt fram till "using namespace oglplus" �r skrivet
// av projektet
void WebcamApp::update() {
	// Bildobjekt dels f�r den rena bilden returnerad fr�n videostr�mmen
	// och dels f�r bilden som skall visas i f�nstret i interfacet
	cv::Mat captureData;
	cv::Mat returnImage;

	// Efterf�ljande kod kommer vara bunden till hur ofta vi kan f� en frame fr�n videoenheten.
	if (captureHandler.GetFrame(captureData)) {
		// Spara valt v�rde f�r kompassens l�gpassfilterfunktion. 
		compass.SetSmoothing(compassCutoff);

		// H�mta senaste v�rde f�r delay mellan bildrutor f�r att anv�nda nedan i filtreringen
		// av avst�ndsber�kningen
		frameDelay = captureHandler.GetDelay();

		// Om bildbehandlingen slutf�rts vill vi hantera informationen och starta en ny omg�ng med 
		// senaste frame.
		if (coords.Ready()) {

			// Avsluta ber�kningstr�den snyggt. Eftersom ber�kningarna �r klara, vilket vi kollade
			// med ready, kommer detta inte att blockera.
			calcThread.join();

			// Beroende p� v�rdet p� variabeln displayMode visar vi olika bilder.
			if (displayMode == 0) {
				returnImage = captureData.clone();
			}
			else if (displayMode == 1) {
				returnImage = coords.GetFilteredImage();
			}

			// H�mta koordinaterna fr�n Coords
			Coords::Coord coord = coords.GetCoords();
			
			// Filtrera avst�ndsv�rde
			// Skapa f�rst en filtreringsparameter baserad p� 
			// nuvarande delay. Vi m�ste reda ut detta regelbundet
			// eftersom uppdateringshastigheten f�r videostr�mmen
			// kan f�r�ndras under k�rning
			// En elegantare l�sning vore att l�gga detta i separat
			// metod men detta implementerades sent i projektet och
			// ingen tid fanns att flytta det till ett b�ttre st�lle
			float L = 2 * PI * frameDelay/1000 * distanceCutoff;
			float smoothing = L/(L + 1);
			
			// Vinkeln objektet upptar, baserat p� bildvinkeln p� kameran
			float ang = (coord.size / returnImage.cols)*horisontalFov*(PI/180);

			// Skillnaden mellan nuvarande ber�knat avst�nd till objektet och
			// f�rra ber�knade v�rdet
			int diff = (objSize / (tan(ang))) - distance;
			
			// filtrerat v�rde p� avst�ndet
			distance += smoothing * diff;

			// Visas bara om en konsoll skapats
			std::cout << "dist: " << std::to_string(distance) 
				<< "      " << "ang: " << std::to_string(L) 
				<< "      " << "smooth: " << std::to_string(smoothing) << "\n";
			
			// Spara koordinaterna i navigationsklassen
			navigator->SetCoords(coord, distance);

			// Om koordinaten var giltig, och vi valt det i inst�llningarna, ritas ett kors ut
			// i returbilden
			if (coord.valid) {
				if (cross) {
					Coords::DrawCross(coord.x, coord.y, returnImage);
				}
			}

			// V�nd bilden r�tt. Den extraheras n�mligen upp-och-ner.
			cv::flip(returnImage.clone(), returnImage, 0);

			// Visa den returnerade bilden.
			imshow("Image", returnImage);

			// Starta ny ber�kningst�d med senaste bildruta som argument.
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Om Coords inte �r ready, men vi inte heller n�gonsin startat, s� starta ber�kningen.
		else if (!started) {
			started = true;
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Nedanst�ende fr�n exempekod
		using namespace oglplus;
		Context::Bound(TextureTarget::_2D, *texture)
			.Image2D(0, PixelDataInternalFormat::RGBA8,
			captureData.cols, captureData.rows, 0,
			PixelDataFormat::BGR, PixelDataType::UnsignedByte,
			captureData.data);
	}
}

// Hantera tangenttryckningar.
void WebcamApp::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_M) {
			// V�xla mellan att visa filtrerad och icke-filtrerad bild
			displayMode = !displayMode;
		}
		if (key == GLFW_KEY_C) {
			// Sl� av och p� utritandet av ett kors
			cross = !cross;
		}
		else if (key == GLFW_KEY_G) {
			// S�g till kvadrokoptern att starta (GO)
			navigator->PrintLine("START");
		}
		else if (key == GLFW_KEY_L) {
			// S�g till kvadrokoptern att landa
			navigator->Land();
		}
		else if (key == GLFW_KEY_S) {
			// S�g till kvadrokoptern att stanna motorerna helt (n�dstopp!)
			navigator->Stop();
		}
		else if (key == GLFW_KEY_R) {
			// Skicka resetkommando till kvadrokoptern
			navigator->PrintLine("RESET");
		}
	}
	RiftApp::onKey(key, scancode, action, mods);
}

// Rendera bild. Taget fr�n exempekoden, med enda modifikationen att 
// bakgrund ej m�las utan s�tts till svart.
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