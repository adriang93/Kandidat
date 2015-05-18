/*

Huvudmodul för programmet. Mycket av koden kommer från exempel 13.2 från OculusRiftInAction,
anpassad för vårt projekt.

https://github.com/OculusRiftInAction/OculusRiftInAction/
Licensierad under Apache 2.0

All kod utom det som markerats skriven av André Wallström.

*/

#include "stdafx.h"
#include "main.h"

// Wrapperkod för att kunna köra CalculateCoords i en separat tråd.
// Bör gå att köra kod från externa klasser i egen tråd utan detta. 
// TODO: Hitta sätt att skippa denna kodsnutt.
void WebcamApp::calcCoordsCall(cv::Mat& image) {
	coords.CalculateCoords(image, filter, interlaced, displayMode);
}

// Konstruktorn är ej tagen från exempelkoden.
WebcamApp::WebcamApp() {

	// Läs värden från en fil. Vi antar att filen är korrekt formaterad.
	// I produktionskod hade detta givetvis varit skrivet med korrekt typechecking.
	// Detta har dock inte prioriterats i projektet.
	std::fstream file("values.txt");
	std::string filedevice;
	file >> filter.lowH >> filter.highH >> filter.lowS >> filter.highS
		>> filter.lowV >> filter.highV >> filedevice >> port
		>> filter.minArea >> filter.maxArea >> filter.minCircularity
		>> filter.open >> filter.close >> objSize >> horisontalFov
		>> distanceCutoff >> compassCutoff >> interlaced >> console;
	//Skapa en WebcamHandler med det device som lästs in från filen.
	try {
		int device = std::stoi(filedevice);
		captureHandler.SetDevice(device);
	}
	// Om vi inte kan konvertera till int tolkar vi värdet som ett filnamn
	catch (...) {
		captureHandler.SetFile(filedevice);
	}

	// Skapa en konsoll (vilket inte görs som stnadard i en Windowsapp) om detta valts via values.txt
	if (console) {
		if (!AllocConsole()) {

			// FAIL är ett makro från OculusRiftInAction-resurserna som låter
			// programmet krascha efter att feltexten visats
			FAIL("Could not create console");
		}

		// Öppna konsollen med magiska värden som gör att det fungerar.
		// från http://stackoverflow.com/questions/9020790/using-stdin-with-an-allocconsole
		freopen("CONOUT$", "w", stdout);
		// Spara ett handle till konsollen så att vi kan modifiera den senare.
		// Används just nu inte alls.
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

// Destruktor. Avslutar startade trådar. Ej tagen från exempelkoden.
WebcamApp::~WebcamApp() {
	delete navigator;

	// Stoppa bildextraheringen. 
	captureHandler.StopCapture();

	// Låt beräkningne slutföras.
	calcThread.join();
}

// Initiera OpenGL-systemet. Det mesta av jobbet görs i moderklassen. 
void WebcamApp::initGl() {

	// Från exempelkoden
	RiftApp::initGl();

	// När initGl körs har moderklassens kod redan initierat Oculus Rift. hmd pekar därför
	// på ett OR-objekt nu. Sätt värdet i kompassmodulen till detta, och starta kompassberäkningen.
	compass.SetHMD(&hmd);
	compass.Start();
	navigator = new NavigatorComm(compass, port);

	// Kod från exempel 13.2, ej modifierad av projektet
	using namespace oglplus;
	texture = TexturePtr(new Texture());
	Context::Bound(TextureTarget::_2D, *texture)
		.MagFilter(TextureMagFilter::Linear)
		.MinFilter(TextureMinFilter::Linear);
	program = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
	float aspectRatio = captureHandler.StartCapture(); // <- startar bildextrahering
	videoGeometry = oria::loadPlane(program, aspectRatio);

	// Justeringar för olika programfunktioner. Skapat av projektet.
	cv::namedWindow("Trackbars", CV_WINDOW_AUTOSIZE);
	// Skapa skjutreglage för alla parametervärden för objektdetekteringen. 
	// Skapa dem med referenser till filterparametrarna så vi inte behöver uppdatera
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
	cv::createTrackbar("Storlek på opening", "Trackbars", &(filter.open), 15);
	cv::createTrackbar("Storlek på closing", "Trackbars", &(filter.close), 15);
	// Skjutreglage för kompassens brytfrekvens.
	cv::createTrackbar("Brytfrekvens för kompass", "Trackbars", &compassCutoff, 500);
	cv::createTrackbar("Brytfrekvens för avstånd", "Trackbars", &distanceCutoff, 10);
}

// Denna kod körs av moderklassen. Allt fram till "using namespace oglplus" är skrivet
// av projektet
void WebcamApp::update() {
	// Bildobjekt dels för den rena bilden returnerad från videoströmmen
	// och dels för bilden som skall visas i fönstret i interfacet
	cv::Mat captureData;
	cv::Mat returnImage;

	// Efterföljande kod kommer vara bunden till hur ofta vi kan få en frame från videoenheten.
	if (captureHandler.GetFrame(captureData)) {
		// Spara valt värde för kompassens lågpassfilterfunktion. 
		compass.SetSmoothing(compassCutoff);

		// Hämta senaste värde för delay mellan bildrutor för att använda nedan i filtreringen
		// av avståndsberäkningen
		frameDelay = captureHandler.GetDelay();

		// Om bildbehandlingen slutförts vill vi hantera informationen och starta en ny omgång med 
		// senaste frame.
		if (coords.Ready()) {

			// Avsluta beräkningstråden snyggt. Eftersom beräkningarna är klara, vilket vi kollade
			// med ready, kommer detta inte att blockera.
			calcThread.join();

			// Beroende på värdet på variabeln displayMode visar vi olika bilder.
			if (displayMode == 0) {
				returnImage = captureData.clone();
			}
			else if (displayMode == 1) {
				returnImage = coords.GetFilteredImage();
			}

			// Hämta koordinaterna från Coords
			Coords::Coord coord = coords.GetCoords();
			
			// Filtrera avståndsvärde
			// Skapa först en filtreringsparameter baserad på 
			// nuvarande delay. Vi måste reda ut detta regelbundet
			// eftersom uppdateringshastigheten för videoströmmen
			// kan förändras under körning
			// En elegantare lösning vore att lägga detta i separat
			// metod men detta implementerades sent i projektet och
			// ingen tid fanns att flytta det till ett bättre ställe
			float L = 2 * PI * frameDelay/1000 * distanceCutoff;
			float smoothing = L/(L + 1);
			
			// Vinkeln objektet upptar, baserat på bildvinkeln på kameran
			float ang = (coord.size / returnImage.cols)*horisontalFov*(PI/180);

			// Skillnaden mellan nuvarande beräknat avstånd till objektet och
			// förra beräknade värdet
			int diff = (objSize / (tan(ang))) - distance;
			
			// filtrerat värde på avståndet
			distance += smoothing * diff;

			// Visas bara om en konsoll skapats
			std::cout << "dist: " << std::to_string(distance) 
				<< "      " << "ang: " << std::to_string(L) 
				<< "      " << "smooth: " << std::to_string(smoothing) << "\n";
			
			// Spara koordinaterna i navigationsklassen
			navigator->SetCoords(coord, distance);

			// Om koordinaten var giltig, och vi valt det i inställningarna, ritas ett kors ut
			// i returbilden
			if (coord.valid) {
				if (cross) {
					Coords::DrawCross(coord.x, coord.y, returnImage);
				}
			}

			// Vänd bilden rätt. Den extraheras nämligen upp-och-ner.
			cv::flip(returnImage.clone(), returnImage, 0);

			// Visa den returnerade bilden.
			imshow("Image", returnImage);

			// Starta ny beräkningståd med senaste bildruta som argument.
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Om Coords inte är ready, men vi inte heller någonsin startat, så starta beräkningen.
		else if (!started) {
			started = true;
			calcThread = std::thread(&WebcamApp::calcCoordsCall, this, captureData);
		}

		// Nedanstående från exempekod
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
			// Växla mellan att visa filtrerad och icke-filtrerad bild
			displayMode = !displayMode;
		}
		if (key == GLFW_KEY_C) {
			// Slå av och på utritandet av ett kors
			cross = !cross;
		}
		else if (key == GLFW_KEY_G) {
			// Säg till kvadrokoptern att starta (GO)
			navigator->PrintLine("START");
		}
		else if (key == GLFW_KEY_L) {
			// Säg till kvadrokoptern att landa
			navigator->Land();
		}
		else if (key == GLFW_KEY_S) {
			// Säg till kvadrokoptern att stanna motorerna helt (nödstopp!)
			navigator->Stop();
		}
		else if (key == GLFW_KEY_R) {
			// Skicka resetkommando till kvadrokoptern
			navigator->PrintLine("RESET");
		}
	}
	RiftApp::onKey(key, scancode, action, mods);
}

// Rendera bild. Taget från exempekoden, med enda modifikationen att 
// bakgrund ej målas utan sätts till svart.
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