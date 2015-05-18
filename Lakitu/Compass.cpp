/*

Kompassmodul. Algoritmen löst baserad på Freescales exempel:

http://cache.freescale.com/files/sensors/doc/app_note/AN4248.pdf

Oculus Rift-koden baserat på Oculus Rift-dokumentationen:

http://static.oculus.com/sdk-downloads/documents/Oculus_Developer_Guide_0.4.4.pdf

All kod utöver det som markerats skriven av André Wallström

*/

#include "stdafx.h"
#include "Compass.h"

// Initerar kompassmodulen med en pekare till ett Oculus Rift-objekt. 
// Detta innebär att vi omedelbart kan starta beräkningen.
Compass::Compass(const ovrHmd* hmd) : hmd(hmd), unfilteredHeading(0), filteredHeading(0) {
	Start();
}

// Initera utan att sätta pekaren till Oculus Rift-objektet. 
// Detta innebär att vi inte kan starta beräkningen, utan det behöver göras manuellt.
Compass::Compass() : hmd(NULL), unfilteredHeading(0), filteredHeading(0) {
}

// Om vi inte har satt pekaren till hmd i konsturktorn behöver vi göra det manuellt.
void Compass::SetHMD(ovrHmd* h) {
	hmd = h;
}

// beräkna utjämningsfaktor från brytfrekvensen som anges. 
// Uppdateringshastigheten är nära 1000 Hz vilket ger deltaT ~= 0.001
void Compass::SetSmoothing(int cutoffFreq) {
	float L = 2 * MATH_FLOAT_PI * 0.001f * cutoffFreq;
	smoothing = L/(L + 1);
}

// Starta kompassextraheringen i en separat tråd. Spara referensen till tråden i variabeln thread.
// Observera: kontrollerar inte om hmd existerar. Det gör istället sensorlooptråden.
void Compass::Start() {
	thread = std::thread(&Compass::SensorLoop, this);
}

// Destruktorn stoppar tråden genom att ta bort referensen till OR. 
// TODO: Avsluta med en bool "calculate" eller liknande istället.
// Vänta på att tråden slutförs innan avslut för att undvika krasch.
Compass::~Compass() {
	this->hmd = NULL;
	thread.join();
}

// Huvudddelen av programmet som extraherar magnetometer- samt accelerometervärden
// från OR, kompenserar för lutning samt beräknar magnetisk norr. Filtrerar även 
// resultatet.
void Compass::SensorLoop() {

	// Om det finns en fil som heter compass.txt skriver vi kompassvärdne till den.
	// För testning.
	std::fstream file("compass.txt");

	// Om det går att läsa filen så innebär det att vi ska skriva till fil i loopen sedan.
	bool write;
	if (file.good()) {
		write = true;
	}
	else {
		write = false;
	}

	// Vi måste ha ett hmd-objekt.
	while (this->hmd) {

		// Varje frame extraheras ett trackingstate-objekt som innehåller momentanvärdet
		// av all sensordata.
		ovrTrackingState tsState = ovrHmd_GetTrackingState(*hmd, ovr_GetTimeInSeconds());

		// HeadPose innehåller huvudposen, alltså riktningen på huvudet så som Oculus Rift 
		// internt tolkar den. 
		OVR::Posef pose = tsState.HeadPose.ThePose;
		ovrVector3f rotation;

		// Spara rotationsaxlarnas värden i vektorn rotation. Efter detta kommer 
		// rotation innehålla pitch, roll och yaw 
		pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&rotation.x, &rotation.y, &rotation.z);

		// Extrahera råa mätvärden från accelerometer och magnetometer.
		ovrVector3f magSensor = tsState.RawSensorData.Magnetometer;
		ovrVector3f accelSensor = tsState.RawSensorData.Accelerometer;

		// Modifiera magnetometervärdena baserat på nuvarande rotation av OR. Detta modifierar
		// vektorn magSensor så den kommer innehålla korrigerade värden efter detta.
		CorrectForTilt(magSensor, rotation);

		// Beräkna magnetisk kompassriktning baserat på momentanvärdena. Detta modifierar inte
		// magSensor.
		unfilteredHeading = CalculateHeading(magSensor);

		// Filtrera headingen med en tidskonstant så att brus och supersnabba rörelser som ändrar
		// accelerometervärdena (och därmed upplevda rotationen) inte påverkar kompassriktningen
		// för mycket. Anpassat från http://cache.freescale.com/files/sensors/doc/app_note/AN4248.pdf

		// filteredHeading kommer att vara förra beräkningsloopens värden (alltså värdet för t-1)
		// så första raden beräknar hur mycket beräknade kompassriktningen ändrats sedan förra
		// loopgenomkörningen.

		float diff = unfilteredHeading - filteredHeading;

		// Om nuvarande värde är 359 grader och förra värdet var 2 grader kommer skillnaden vara 
		// väldigt stor, 357 grader. Egentligen är ju dock skillnaden bara 3 grader. Vi behöver därför
		// justera differensen för om värdet passerat noll grader så vi får dne riktiga differensen. 
		if (diff < -180) {
			diff += 360;
		}
		else  if (diff > 180) {
			diff -= 360;
		}

		// Filtrera med tidskonstanten. Brytfrekvensen sätts av kallande klass med SetCutoff.
		// Lågpassfilter där utjämningsfaktorn är smoothing.
		filteredHeading += (smoothing * diff);

		// Justera tillbaka om värdet återigne slagit runt nollan.
		if (filteredHeading  > 360) {
			filteredHeading  -= 360;
		}
		else if (filteredHeading  < 0) {
			filteredHeading  += 360;
		}

		// Skriv testvärden till fil om compass.txt ligger i samma mapp som programmet
		// Skriver ut samtliga värden och används för testning.
		if (write) {
			file << std::to_string(filteredHeading) << ", " 
				<< std::to_string(unfilteredHeading) << "\n";
		}

		// Hur många millisekunder skall det sovas? Sensorerna uppdateras med 1kHz så vi 
		// väntar en millisekund.
		Sleep(1);
	}
}

// Returnerar den ofiltrerade kompassriktningen
float Compass::UnFilteredHeading() const {
	return unfilteredHeading;
}

// Returnerar den filtrerade kompassriktningen
float Compass::FilteredHeading() const {
	return filteredHeading;
}

// Beräkna kompassriktning utifrån magnetometervärden. Kompenserar ej för 
// lutning.
float Compass::CalculateHeading(const ovrVector3f& magSensor) {
	// Om samtliga värden är noll kommer ingen av if-satserna nedan vara tillfredsställda, 
	// och riktningen behöver redan vara bestämd. Händer i princip aldrig i praktisk drift, 
	// men händer konstant om man till exempel använder emulerad Oculur Rift.
	float direction = 0;

	// Beräknar vinkeln mellan de olika axlarna med tangens. Enbart axlarna i horisontalplanet behöver 
	// användas då det ju är de som visar riktningen mot magnetisk norr. 
	if (magSensor.x > 0) {
		direction = 270 - atan(magSensor.z / magSensor.x) * 180 / MATH_FLOAT_PI;
	}
	else if (magSensor.x < 0) {
		direction = 90 - atan(magSensor.z / magSensor.x) * 180 / MATH_FLOAT_PI;
	}
	else if ((magSensor.x == 0) & (magSensor.z < 0)) {
		direction = 0;
	}
	else if ((magSensor.x == 0) & (magSensor.z > 0)) {
		direction = 180;
	}
	return direction;
}

// Korrigera värdena i magSensor för rotationen från rotation
void Compass::CorrectForTilt(ovrVector3f& magSensor, const ovrVector3f& rotation) {
	ovrVector3f corr; // Korrigerade värden

	// Eftersom vi använder cos och sin flera gånger är det effektivare att beräkna dem i
	// förväg. 
	float cosVal = cos(rotation.z);
	float sinVal = sin(rotation.z);

	// Kompensera x-axeln baserat på lutningen av y-axeln (vertikalaxeln). 
	// Kompensera även y-axeln för att användas för beräkningarna av z-axeln senare.
	corr.x = magSensor.x * cosVal - magSensor.y * sinVal;
	corr.y = magSensor.y * cosVal + magSensor.x * sinVal;

	sinVal = sin(rotation.y);
	cosVal = cos(rotation.y);

	// Kompensera z-axeln baserat på den redan kompenserade y-axeln.
	corr.z = magSensor.z * cosVal + corr.y * sinVal;

	magSensor.x = corr.x;
	magSensor.y = corr.y;
	magSensor.z = corr.z;
}