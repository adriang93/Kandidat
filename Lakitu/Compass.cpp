/*

Kompassmodul. Algoritmen l�st baserad p� Freescales exempel:

http://cache.freescale.com/files/sensors/doc/app_note/AN4248.pdf

Oculus Rift-koden baserat p� minimal Oculus Rift example.

TODO: L�nk till licens.

*/

#include "stdafx.h"
#include "Compass.h"

// Initerar kompassmodulen med en pekare till ett Oculus Rift-objekt. 
// Detta inneb�r att vi omedelbart kan starta ber�kningen.
Compass::Compass(const ovrHmd* hmd) : hmd(hmd), unfilteredHeading(0), filteredHeading(0) {
	Start();
}

// Initera utan att s�tta pekaren till Oculus Rift-objektet. 
// Detta inneb�r att vi inte kan starta ber�kningen, utan det beh�ver g�ras manuellt.
Compass::Compass() : hmd(NULL), unfilteredHeading(0), filteredHeading(0) {
}

// Om vi inte har satt pekaren till hmd i konsturktorn beh�ver vi g�ra det manuellt.
void Compass::SetHMD(ovrHmd* h) {
	hmd = h;
}

// ber�kna utj�mningsfaktor fr�n brytfrekvensen som anges. 
// Uppdateringshastigheten �r n�ra 1000 Hz vilket ger deltaT ~= 0.001
void Compass::SetSmoothing(int cutoffFreq) {
	float L = 2 * MATH_FLOAT_PI * 0.001 * cutoffFreq;
	smoothing = (L) / (L + 1);
}

// Starta kompassextraheringen i en separat tr�d. Spara referensen till tr�den i variabeln thread.
// Observera: kontrollerar inte om hmd existerar. Det g�r ist�llet sensorlooptr�den.
void Compass::Start() {
	thread = std::thread(&Compass::SensorLoop, this);
}

// Destruktorn stoppar tr�den genom att ta bort referensen till OR. 
// TODO: Avsluta med en bool "calculate" eller liknande ist�llet.
// V�nta p� att tr�den slutf�rs innan avslut f�r att undvika krasch.
Compass::~Compass() {
	this->hmd = NULL;
	thread.join();
}

// Huvudddelen av programmet som extraherar magnetometer- samt accelerometerv�rden
// fr�n OR, kompenserar f�r lutning samt ber�knar magnetisk norr. Filtrerar �ven 
// resultatet.
void Compass::SensorLoop() {

	// Om det finns en fil som heter compass.txt skriver vi kompassv�rdne till den.
	// F�r testning.
	std::fstream file("compass.txt");
	bool write;
	if (file.good()) {
		write = true;
	}
	else {
		write = false;
	}

	// Vi m�ste ha ett hmd-objekt.
	while (this->hmd) {

		// Varje frame extraheras ett trackingstate-objekt som inneh�ller momentanv�rdet
		// av all sensordata.
		ovrTrackingState tsState = ovrHmd_GetTrackingState(*hmd, ovr_GetTimeInSeconds());

		// HeadPose inneh�ller huvudposen, allts� riktningen p� huvudet s� som Oculus Rift 
		// internt tolkar den. 
		OVR::Posef pose = tsState.HeadPose.ThePose;
		ovrVector3f rotation;

		// Spara rotationsaxlarnas v�rden i vektorn rotation. Efter detta kommer 
		// rotation inneh�lla pitch, roll och yaw (kanske inte i den ordningen; kommer ej ih�g)
		pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&rotation.x, &rotation.y, &rotation.z);

		// Extrahera r�a m�tv�rden fr�n accelerometer och magnetometer.
		ovrVector3f magSensor = tsState.RawSensorData.Magnetometer;
		ovrVector3f accelSensor = tsState.RawSensorData.Accelerometer;

		// Modifiera magnetometerv�rdena baserat p� nuvarande rotation av OR. Detta modifierar
		// vektorn magSensor s� den kommer inneh�lla korrigerade v�rden efter detta.
		CorrectForTilt(magSensor, rotation);

		// Ber�kna magnetisk kompassriktning baserat p� momentanv�rdena. Detta modifierar inte
		// magSensor.
		unfilteredHeading = CalculateHeading(magSensor);

		// Filtrera headingen med en tidskonstant s� att brus och supersnabba r�relser som �ndrar
		// accelerometerv�rdena (och d�rmed upplevda rotationen) inte p�verkar kompassriktningen
		// f�r mycket. Anpassat fr�n http://cache.freescale.com/files/sensors/doc/app_note/AN4248.pdf

		// filteredHeading kommer att vara f�rra ber�kningsloopens v�rden (allts� v�rdet f�r t-1)
		// s� f�rsta raden ber�knar hur mycket ber�knade kompassriktningen �ndrats sedan f�rra
		// loopgenomk�rningen.

		float diff = unfilteredHeading - filteredHeading;

		// Om nuvarande v�rde �r 359 grader och f�rra v�rdet var 2 grader kommer skillnaden vara 
		// v�ldigt stor, 357 grader. Egentligen �r ju dock skillnaden bara 3 grader. Vi beh�ver d�rf�r
		// justera differensen f�r om v�rdet passerat noll grader s� vi f�r dne riktiga differensen. 
		if (diff < -180) {
			diff += 360;
		}
		else  if (diff > 180) {
			diff -= 360;
		}

		// Filtrera med tidskonstanten. Brytfrekvensen s�tts av kallande klass med SetCutoff.
		// L�gpassfilter d�r utj�mningsfaktorn �r smoothing.
		diff += smoothing * diff;

		// Justera tillbaka om v�rdet �terigne slagit runt nollan.
		if (diff > 360) {
			diff -= 360;
		}
		else if (diff < 0) {
			diff += 360;
		}
		filteredHeading = diff;

		// Skriv testv�rden till fil om compass.txt ligger i samma mapp som programmet
		if (write) {
			file << std::to_string(filteredHeading) << ", " << std::to_string(unfilteredHeading) << "\n";
		}

		// Hur m�nga g�nger per sekund skall kompassen uppdateras?
		Sleep(1);
	}
}

float Compass::UnFilteredHeading() const {
	return unfilteredHeading;
}

float Compass::FilteredHeading() const {
	return filteredHeading;
}

// Ber�kna kompassriktning utifr�n magnetometerv�rden. Kompenserar ej f�r 
// lutning.
float Compass::CalculateHeading(const ovrVector3f& magSensor) {
	// Om samtliga v�rden �r noll kommer ingen av if-satserna nedan vara tillfredsst�llda, 
	// och riktningen beh�ver redan vara best�md. H�nder i princip aldrig i praktisk drift, 
	// men h�nder konstant om man till exempel anv�nder emulerad Oculur Rift.
	float direction = 0;
	const float PI = 3.1459f;

	// Ber�knar vinkeln mellan de olika axlarna med tangens. Enbart axlarna i horisontalplanet beh�ver 
	// anv�ndas d� det ju �r de som visar riktningen mot magnetisk norr. 
	if (magSensor.x > 0) {
		direction = 270 - atan(magSensor.z / magSensor.x) * 180 / PI;
	}
	else if (magSensor.x < 0) {
		direction = 90 - atan(magSensor.z / magSensor.x) * 180 / PI;
	}
	else if ((magSensor.x == 0) & (magSensor.z < 0)) {
		direction = 0;
	}
	else if ((magSensor.x == 0) & (magSensor.z > 0)) {
		direction = 180;
	}
	return direction;
}

// Korrigera v�rdena i magSensor f�r rotationen fr�n rotation
void Compass::CorrectForTilt(ovrVector3f& magSensor, const ovrVector3f& rotation) {
	ovrVector3f corr; // Korrigerade v�rden

	// Eftersom vi anv�nder cos och sin flera g�nger �r det effektivare att ber�kna dem i
	// f�rv�g. 
	float cosVal = cos(rotation.z);
	float sinVal = sin(rotation.z);

	// Kompensera x-axeln baserat p� lutningen av y-axeln (vertikalaxeln). Enkel trigonometri.
	// Kompensera �ven y-axeln f�r att anv�ndas f�r ber�kningarna av z-axeln senare.
	corr.x = magSensor.x * cosVal - magSensor.y * sinVal;
	corr.y = magSensor.y * cosVal + magSensor.x * sinVal;

	sinVal = sin(rotation.y);
	cosVal = cos(rotation.y);

	// Kompensera z-axeln baserat p� den redan kompenserade y-axeln.
	corr.z = magSensor.z * cosVal + corr.y * sinVal;
	magSensor.x = corr.x;
	magSensor.y = corr.y;
	magSensor.z = corr.z;

	//magSensor = corr; <- kortare kod. Otestad, men borde funka. 
}