
#include <WinSock2.h>
#include <Windows.h>

#include "DataPlugin.hpp"          // corresponding header file
#include <math.h>               // for atan2, sqrt
#include <stdio.h>              // for sample output
#include <assert.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <WS2tcpip.h>


#define TIME_LENGTH 26
#define MAX_PACKET_LEN 32768

DataPlugin::DataPlugin()
{
}

DataPlugin::~DataPlugin()
{
}

// plugin information

extern "C" __declspec(dllexport)
const char * __cdecl GetPluginName()                   { return("rf2livetiming"); }

extern "C" __declspec(dllexport)
PluginObjectType __cdecl GetPluginType()               { return(PO_INTERNALS); }

extern "C" __declspec(dllexport)
int __cdecl GetPluginVersion()                         { return(6); }

extern "C" __declspec(dllexport)
PluginObject * __cdecl CreatePluginObject()            { return((PluginObject *) new DataPlugin); }

extern "C" __declspec(dllexport)
void __cdecl DestroyPluginObject(PluginObject *obj)  { delete((DataPlugin *)obj); }




void DataPlugin::log(const char *msg) {
	FILE *logFile;
	time_t curtime;
	struct tm loctime;
	char thetime[TIME_LENGTH];


	int err = fopen_s(&logFile, "UserData\\Log\\rf2livetiming.log", "a");
	if (err == 0) {
		curtime = time(NULL);
		int err2 = localtime_s(&loctime, &curtime);
		int err3 = asctime_s(thetime, TIME_LENGTH, &loctime);
		thetime[TIME_LENGTH - 2] = 0;
		fprintf(logFile, "[%s] %s\n", thetime, msg);
		fclose(logFile);
	}
}

void DataPlugin::Startup(long version)
{
	// marrs:
	FILE *settings;
	//struct hostent *ptrh;
	data_version = 1;
	char portstring[10];
	char iniip[16];
	int localhost;
	int errcode;

	ADDRINFO hints = { sizeof(addrinfo) };
	hints.ai_flags = AI_ALL;
	hints.ai_family = PF_INET;
	hints.ai_protocol = IPPROTO_IPV4;
	ADDRINFO *pResult = NULL;

	log("starting plugin");

	// open socket
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		log("could not create datagram socket");
		return;
	}
	int err = fopen_s(&settings, "rf2livetiming.ini", "r");
	//err = 1;
	if (err == 0) {
		log("reading settings");
		/*if (fscanf_s(settings, "%[^:]:%i", hostname, _countof(hostname), &port) != 2) {
			log("could not read host and port");
		}
		//ptrh = gethostbyname(hostname);
		log("settings read from file");
		int errcode = getaddrinfo(hostname, NULL, &hints, &pResult);
		*/
		if (fscanf_s(settings, "USE LOCALHOST=\"%i\"\n", &localhost) != 1) {
			log("could not read localhost flag, using default: 1");
			localhost = 1;
		} else if (localhost)
			log("localhost");
		if (fscanf_s(settings, "DEST IP=\"%[^\"]\"\n", iniip, _countof(iniip)) != 1) {
			if(localhost)
				log("could not read IP, but using localhost");
			else
				log("could not read IP, using default: 127.0.0.1");
		} else if(!localhost)
			log(iniip);
		if (fscanf_s(settings, "DEST PORT=\"%i\"", &port) != 1) {
			log("could not read port, using default: 6789");
			port = 6789;
		} else {
			sprintf_s(portstring, "%i", port);
			log(portstring);
		}
		fclose(settings);
		
		if(localhost)
			errcode = getaddrinfo("localhost", NULL, &hints, &pResult);
		else
			errcode = getaddrinfo(iniip, NULL, &hints, &pResult);
	}
	else {
		log("could not read settings, using defaults: localhost:6789");
		//ptrh = gethostbyname("localhost"); /* Convert host name to equivalent IP address and copy to sad. */

		errcode = getaddrinfo("localhost", NULL, &hints, &pResult);


		port = 6789;
	}
	memset((char *)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET;           /* set family to Internet     */
	sad.sin_port = htons((u_short)port);
	//if (((char *)ptrh) == NULL) {
	//	log("invalid host name");
	//	return;
	//}



	//memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	//memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	sad.sin_addr.S_un.S_addr = *((ULONG*)&(((sockaddr_in*)pResult->ai_addr)->sin_addr));


}


void DataPlugin::Shutdown()
{
	if (s > 0) {
		closesocket(s);
		s = 0;
	}
}


void DataPlugin::Load()
{
}


void DataPlugin::Unload()
{
}


void DataPlugin::EnterRealtime()
{
	// marrs:
	mET = 0.0f;
}


void DataPlugin::ExitRealtime()
{
	// marrs:
	mET = -1.0f;
}


void DataPlugin::StartSession()
{
}


void DataPlugin::EndSession()
{
}


void DataPlugin::UpdateScoring(const ScoringInfoV01 &info)
{
	// marrs:
	//log("starting update");
	StartStream();
	StreamData((char *)&type_scoring, sizeof(char));

	// session data (changes mostly with changing sessions)
	StreamString((char *)&info.mTrackName, 64);
	StreamData((char *)&info.mSession, sizeof(long));

	// event data (changes continuously)
	StreamData((char *)&info.mCurrentET, sizeof(double));
	StreamData((char *)&info.mEndET, sizeof(double));
	StreamData((char *)&info.mMaxLaps, sizeof(long));
	StreamData((char *)&info.mLapDist, sizeof(double));
	StreamData((char *)&info.mNumVehicles, sizeof(long));

	StreamData((char *)&info.mGamePhase, sizeof(byte));
	StreamData((char *)&info.mYellowFlagState, sizeof(byte));
	StreamData((char *)&info.mSectorFlag[0], sizeof(byte));
	StreamData((char *)&info.mSectorFlag[1], sizeof(byte));
	StreamData((char *)&info.mSectorFlag[2], sizeof(byte));
	StreamData((char *)&info.mStartLight, sizeof(byte));
	StreamData((char *)&info.mNumRedLights, sizeof(byte));

	// scoring data (changes with new sector times)
	for (long i = 0; i < info.mNumVehicles; i++) {
		VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
		StreamData((char *)&vinfo.mPos.x, sizeof(double));
		StreamData((char *)&vinfo.mPos.z, sizeof(double));
		StreamData((char *)&vinfo.mPlace, sizeof(char));
		StreamData((char *)&vinfo.mLapDist, sizeof(double));
		StreamData((char *)&vinfo.mPathLateral, sizeof(double));
		const double metersPerSec = sqrt((vinfo.mLocalVel.x * vinfo.mLocalVel.x) +
			(vinfo.mLocalVel.y * vinfo.mLocalVel.y) +
			(vinfo.mLocalVel.z * vinfo.mLocalVel.z));
		StreamData((char *)&metersPerSec, sizeof(double));
		StreamString((char *)&vinfo.mVehicleName, 64);
		StreamString((char *)&vinfo.mDriverName, 32);
		StreamString((char *)&vinfo.mVehicleClass, 32);
		StreamData((char *)&vinfo.mTotalLaps, sizeof(short));
		StreamData((char *)&vinfo.mBestSector1, sizeof(double));
		StreamData((char *)&vinfo.mBestSector2, sizeof(double));
		StreamData((char *)&vinfo.mBestLapTime, sizeof(double));
		StreamData((char *)&vinfo.mLastSector1, sizeof(double));
		StreamData((char *)&vinfo.mLastSector2, sizeof(double));
		StreamData((char *)&vinfo.mLastLapTime, sizeof(double));
		StreamData((char *)&vinfo.mCurSector1, sizeof(double));
		StreamData((char *)&vinfo.mCurSector2, sizeof(double));
		StreamData((char *)&vinfo.mTimeBehindLeader, sizeof(double));
		StreamData((char *)&vinfo.mLapsBehindLeader, sizeof(long));
		StreamData((char *)&vinfo.mTimeBehindNext, sizeof(double));
		StreamData((char *)&vinfo.mLapsBehindNext, sizeof(long));
		StreamData((char *)&vinfo.mNumPitstops, sizeof(short));
		StreamData((char *)&vinfo.mNumPenalties, sizeof(short));
		StreamData((char *)&vinfo.mInPits, sizeof(bool));
		StreamData((char *)&vinfo.mSector, sizeof(char));
		StreamData((char *)&vinfo.mFinishStatus, sizeof(char));
	}
	StreamVarString((char *)info.mResultsStream);
	EndStream();
	//log("ending update\n");
}


void DataPlugin::UpdateTelemetry(const TelemInfoV01 &info)
{
	// marrs:
	//log("starting telemetry");
	StartStream();
	StreamData((char *)&type_telemetry, sizeof(char));
	StreamData((char *)&info.mGear, sizeof(long));
	StreamData((char *)&info.mEngineRPM, sizeof(double));
	StreamData((char *)&info.mEngineMaxRPM, sizeof(double));
	StreamData((char *)&info.mEngineWaterTemp, sizeof(double));
	StreamData((char *)&info.mEngineOilTemp, sizeof(double));
	StreamData((char *)&info.mClutchRPM, sizeof(double));
	StreamData((char *)&info.mOverheating, sizeof(bool));
	StreamData((char *)&info.mFuel, sizeof(double));

	StreamData((char *)&info.mPos.x, sizeof(double));
	StreamData((char *)&info.mPos.y, sizeof(double));
	StreamData((char *)&info.mPos.z, sizeof(double));

	const double metersPerSec = sqrt((info.mLocalVel.x * info.mLocalVel.x) +
		(info.mLocalVel.y * info.mLocalVel.y) +
		(info.mLocalVel.z * info.mLocalVel.z));
	StreamData((char *)&metersPerSec, sizeof(double));

	StreamData((char *)&info.mLapStartET, sizeof(double));
	StreamData((char *)&info.mLapNumber, sizeof(long));

	StreamData((char *)&info.mUnfilteredThrottle, sizeof(double));
	StreamData((char *)&info.mUnfilteredBrake, sizeof(double));
	StreamData((char *)&info.mUnfilteredSteering, sizeof(double));
	StreamData((char *)&info.mUnfilteredClutch, sizeof(double));

	StreamData((char *)&info.mLastImpactET, sizeof(double));
	StreamData((char *)&info.mLastImpactMagnitude, sizeof(double));
	StreamData((char *)&info.mLastImpactPos.x, sizeof(double));
	StreamData((char *)&info.mLastImpactPos.y, sizeof(double));
	StreamData((char *)&info.mLastImpactPos.z, sizeof(double));
	for (long i = 0; i < 8; i++) {
		StreamData((char *)&info.mDentSeverity[i], sizeof(byte));
	}

	for (long i = 0; i < 4; i++) {
		const TelemWheelV01 &wheel = info.mWheel[i];
		StreamData((char *)&wheel.mDetached, sizeof(bool));
		StreamData((char *)&wheel.mFlat, sizeof(bool));
		StreamData((char *)&wheel.mBrakeTemp, sizeof(double));
		StreamData((char *)&wheel.mPressure, sizeof(double));
		StreamData((char *)&wheel.mRideHeight, sizeof(double));
		StreamData((char *)&wheel.mTemperature[0], sizeof(double));
		StreamData((char *)&wheel.mTemperature[1], sizeof(double));
		StreamData((char *)&wheel.mTemperature[2], sizeof(double));
		StreamData((char *)&wheel.mWear, sizeof(double));
	}
	EndStream();
	//log("ending telemetry\n");
}


void DataPlugin::Error(const char * const msg)
{
	log(msg);
}




bool DataPlugin::WantsToDisplayMessage(MessageInfoV01 &msgInfo) {
	return false;
}

unsigned char DataPlugin::WantsToViewVehicle(CameraControlInfoV01 &camControl) {
	return 0;
}


// marrs:
void DataPlugin::StartStream() {
	data_packet = 0;
	data_sequence++;
	data[0] = data_version;
	data[1] = data_packet;
	memcpy(&data[2], &data_sequence, sizeof(short));
	data_offset = 4;
}

void DataPlugin::StreamData(char *data_ptr, int length) {
	int i;

	for (i = 0; i < length; i++) {
		if (data_offset + i == MAX_PACKET_LEN) {
			sendto(s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &sad, sizeof(struct sockaddr));
			data_packet++;
			data[0] = data_version;
			data[1] = data_packet;
			memcpy(&data[2], &data_sequence, sizeof(short));
			data_offset = 4;
			length = length - i;
			data_ptr += i;
			i = 0;
		}
		data[data_offset + i] = data_ptr[i];
	}
	data_offset = data_offset + length;
}

void DataPlugin::StreamVarString(char *data_ptr) {
	int i = 0;
	while (data_ptr[i] != 0) {
		i++;
	}
	StreamData((char *)&i, sizeof(int));
	StreamString(data_ptr, i);
}

void DataPlugin::StreamString(char *data_ptr, int length) {
	int i;

	for (i = 0; i < length; i++) {
		if (data_offset + i == MAX_PACKET_LEN) {
			sendto(s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &sad, sizeof(struct sockaddr));
			data_packet++;
			data[0] = data_version;
			data[1] = data_packet;
			memcpy(&data[2], &data_sequence, sizeof(short));
			data_offset = 4;
			length = length - i;
			data_ptr += i;
			i = 0;
		}
		data[data_offset + i] = data_ptr[i];
		if (data_ptr[i] == 0) {
			// found end of string, so this is where we stop
			data_offset = data_offset + i + 1;
			return;
		}
	}
	data_offset = data_offset + length;
}

void DataPlugin::EndStream() {
	if (data_offset > 4) {
		sendto(s, data, data_offset, 0, (struct sockaddr *) &sad, sizeof(struct sockaddr));
	}
}
