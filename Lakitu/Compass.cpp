#include "stdafx.h"
#include "Compass.h"

Compass::Compass(const ovrHmd* hmd) : hmd(hmd), unfilteredHeading(0), filteredHeading(0) {
	thread = std::thread(&Compass::SensorLoop, this);
}

Compass::~Compass() {
	this->hmd = NULL;
	thread.join();
}

void Compass::SensorLoop() {
	ovrFrameTiming frameTiming;
	while (this->hmd) {
		frameTiming = ovrHmd_BeginFrameTiming(*hmd, 0);

		ovrTrackingState tsState = ovrHmd_GetTrackingState(*hmd, frameTiming.ScanoutMidpointSeconds);
		// The cpp compatibility layer is used to convert ovrPosef to Posef (see OVR_Math.h)
		OVR::Posef pose = tsState.HeadPose.ThePose;
		ovrVector3f rotation;
		pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&rotation.x, &rotation.y, &rotation.z);

		ovrVector3f magSensor = tsState.RawSensorData.Magnetometer;
		ovrVector3f accelSensor = tsState.RawSensorData.Accelerometer;

		CorrectForTilt(magSensor, rotation);
		unfilteredHeading = CalculateHeading(magSensor);

		float diff = unfilteredHeading - filteredHeading;
		if (diff < -180) {
			diff += 360;
		} else  if (diff > 180) {
			diff -= 360;
		}
		diff = filteredHeading + (0.1f * diff);
		if (diff > 360) {
			diff -= 360;
		} else if (diff < 0) {
			diff += 360;
		}
		filteredHeading = diff;

		Sleep(5);
		if (this->hmd) {
			ovrHmd_EndFrameTiming(*hmd);
		}
	}
}

float Compass::UnFilteredHeading() {
	return unfilteredHeading;
}

float Compass::FilteredHeading() {
	return filteredHeading;
}

float Compass::CalculateHeading(const ovrVector3f& magSensor) {
	float direction = 0;
	const float PI = 3.1459f;

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

void Compass::CorrectForTilt(ovrVector3f& magSensor, const ovrVector3f& rotation) {
	ovrVector3f corr;
	float cosVal = cos(rotation.z);
	float sinVal = sin(rotation.z);
	corr.x = magSensor.x * cosVal - magSensor.y * sinVal;
	corr.y = magSensor.y * cosVal + magSensor.x * sinVal;

	sinVal = sin(rotation.y);
	cosVal = cos(rotation.y);

	corr.z = magSensor.z * cosVal + corr.y * sinVal;
	magSensor.x = corr.x;
	magSensor.y = corr.y;
	magSensor.z = corr.z;
}