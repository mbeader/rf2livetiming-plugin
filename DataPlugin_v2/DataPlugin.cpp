
#include <WinSock2.h>
#include <Windows.h>

#include "DataPlugin.hpp"          // corresponding header file
#include <math.h>               // for atan2, sqrt
#include <assert.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <WS2tcpip.h>


#define TIME_LENGTH 26
#define MAX_PACKET_LEN 32768
#define TCP_PACKET_LEN 512


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
int __cdecl GetPluginVersion()                         { return(7); }

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
	last_check = 0;
	char portstring[10];
	char iniip[16];
	int localhost;
	int send_results;
	int errcode;

	ADDRINFO udp_hints = { sizeof(addrinfo) };
	udp_hints.ai_flags = AI_ALL;
	udp_hints.ai_family = PF_INET;
	udp_hints.ai_protocol = IPPROTO_IPV4;
	ADDRINFO *udp_pResult = NULL;

	ADDRINFO tcp_hints = { sizeof(addrinfo) };
	tcp_hints.ai_flags = AI_ALL;
	tcp_hints.ai_family = PF_INET;
	tcp_hints.ai_protocol = IPPROTO_IPV4;
	ADDRINFO *tcp_pResult = NULL;

	log("starting plugin");

	// open socket
	udp_s = socket(PF_INET, SOCK_DGRAM, 0);
	if(udp_s < 0) {
		log("could not create datagram socket");
		return;
	}
	int err = fopen_s(&settings, "rf2livetiming.ini", "r");
	//err = 1;
	if(err == 0) {
		log("reading settings");
		if(fscanf_s(settings, "USE LOCALHOST=\"%i\"\n", &localhost) != 1) {
			log("could not read localhost flag, using default: 1");
			localhost = 1;
		} else if (localhost)
			log("localhost");
		if(fscanf_s(settings, "DEST IP=\"%[^\"]\"\n", iniip, _countof(iniip)) != 1) {
			if(localhost)
				log("could not read IP, but using localhost");
			else
				log("could not read IP, using default: 127.0.0.1");
		} else if(!localhost)
			log(iniip);
		if(fscanf_s(settings, "DEST PORT=\"%i\"\n", &port) != 1) {
			log("could not read port, using default: 6789");
			port = 6789;
		} else {
			sprintf_s(portstring, "%i", port);
			log(portstring);
		}
		if(fscanf_s(settings, "SEND RESULTS=\"%i\"", &send_results) != 1) {
			log("could not read send results flag, using default: 1");
			send_results = 1;
		} else if(send_results)
			log("Will send results files");
		fclose(settings);
		
		if(localhost)
			errcode = getaddrinfo("localhost", NULL, &udp_hints, &udp_pResult);
		else
			errcode = getaddrinfo(iniip, NULL, &udp_hints, &udp_pResult);
	}
	else {
		log("could not read settings, using defaults: localhost:6789");
		errcode = getaddrinfo("localhost", NULL, &udp_hints, &udp_pResult);
		port = 6789;
	}
	memset((char *)&udp_sad, 0, sizeof(udp_sad)); /* clear sockaddr structure */
	udp_sad.sin_family = AF_INET;           /* set family to Internet     */
	udp_sad.sin_port = htons((u_short)port);
	
	if(send_results) {
		tcp_s = socket(PF_INET, SOCK_STREAM, 0);
		if(tcp_s < 0) {
			log("could not create stream socket");
			return;
		}
		if(localhost)
			errcode = getaddrinfo("localhost", NULL, &tcp_hints, &tcp_pResult);
		else
			errcode = getaddrinfo(iniip, NULL, &tcp_hints, &tcp_pResult);
		memset((char *)&tcp_sad, 0, sizeof(tcp_sad)); /* clear sockaddr structure */
		tcp_sad.sin_family = AF_INET;           /* set family to Internet     */
		tcp_sad.sin_port = htons((u_short)port);
		tcp_sad.sin_addr.S_un.S_addr = *((ULONG*)&(((sockaddr_in*)tcp_pResult->ai_addr)->sin_addr));
	}


	//memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	//memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	udp_sad.sin_addr.S_un.S_addr = *((ULONG*)&(((sockaddr_in*)udp_pResult->ai_addr)->sin_addr));
}


void DataPlugin::Shutdown()
{
	if(udp_s > 0) {
		closesocket(udp_s);
		udp_s = 0;
	}
	if(tcp_s > 0) {
		closesocket(tcp_s);
		tcp_s = 0;
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
	time_t curr;
	time(&curr);
	time_t results_time;
	char results_filename[29];

	if(last_check) {
		FindNewResult(results_filename);
		if(results_filename[0]) {
			results_time = ParseResultsTime(results_filename);
			if(results_time > last_check)
				ReadResults(results_filename);
			else
				log("old result");
		}
	}
	last_check = curr;
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

	// multiplayer data
	StreamData((char *)&info.mServerPublicIP, sizeof(long));
	StreamData((char *)&info.mServerPort, sizeof(short));
	StreamString((char *)&info.mServerName, 32);
	StreamData((char *)&info.mMaxPlayers, sizeof(long));
	StreamData((char *)&info.mStartET, sizeof(float));

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
	for(long i = 0; i < info.mNumVehicles; i++) {
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
		StreamData((char *)&vinfo.mControl, sizeof(char));
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
	for(long i = 0; i < 8; i++) {
		StreamData((char *)&info.mDentSeverity[i], sizeof(byte));
	}

	for(long i = 0; i < 4; i++) {
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

	for(i = 0; i < length; i++) {
		if(data_offset + i == MAX_PACKET_LEN) {
			sendto(udp_s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
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
	while(data_ptr[i] != 0) {
		i++;
	}
	StreamData((char *)&i, sizeof(int));
	StreamString(data_ptr, i);
}

void DataPlugin::StreamString(char *data_ptr, int length) {
	int i;

	for(i = 0; i < length; i++) {
		if(data_offset + i == MAX_PACKET_LEN) {
			sendto(udp_s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
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
		if(data_ptr[i] == 0) {
			// found end of string, so this is where we stop
			data_offset = data_offset + i + 1;
			return;
		}
	}
	data_offset = data_offset + length;
}

void DataPlugin::EndStream() {
	if(data_offset > 4) {
		sendto(udp_s, data, data_offset, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
	}
}

void DataPlugin::FindNewResult(char *name) {
	WIN32_FIND_DATA data;
	HANDLE hand = FindFirstFile(".\\UserData\\Log\\Results\\*.xml", &data);

	if(hand != INVALID_HANDLE_VALUE) {
		do {
			strncpy_s(name, 29, (const char *) &data.cFileName, 29);
		} while(FindNextFile(hand, &data));
		FindClose(hand);
	}
}

/*void DataPlugin::FindNewResults() {
	WIN32_FIND_DATA data;
	HANDLE hand = FindFirstFile(".\\UserData\\Log\\Results\\*.xml", &data);

	if(hand != INVALID_HANDLE_VALUE) {
		do {
			log(data.cFileName);
		} while(FindNextFile(hand, &data));
		FindClose(hand);
	}
}*/

time_t DataPlugin::ParseResultsTime(char *name) {
	struct tm t;
	t.tm_year = atoi(name) - 1900;
	t.tm_mon = atoi(name + 5) - 1;
	t.tm_mday = atoi(name + 8);
	t.tm_hour = atoi(name + 11);
	t.tm_min = atoi(name + 14);
	t.tm_sec = atoi(name + 17);
	return mktime(&t);
}

void DataPlugin::ReadResults(char *name) {
	FILE *r;
	char path[52];
	strncpy_s(path, 52, ".\\UserData\\Log\\Results\\0000_00_00_00_00_00-00R1.xml", 52);
	strncpy_s(path+23, 29, name, 29);
	if(fopen_s(&r, path, "rb") == 0) {
		if(SendResults(&r))
			log("failed to send results file");
		fclose(r);
	} else
		log("cannot open results file");
}

int DataPlugin::SendResults(FILE** r) {
	char buf[TCP_PACKET_LEN];
	size_t nread;
	int nsent;

	if(connect(tcp_s, (struct sockaddr *) &tcp_sad, sizeof(struct sockaddr)))
		return -1;
	log("connection successful");
	while((nread = fread(buf, sizeof(char), TCP_PACKET_LEN, *r)) > 0) {
		nsent = send(tcp_s, buf, (int) nread, 0);
		if(nsent < 0)
			return -2;
		if(nsent < nread)
			log("did not send all bytes");
		log("sent packet");
	}

	shutdown(tcp_s, SD_BOTH);
	closesocket(tcp_s);
	tcp_s = socket(PF_INET, SOCK_STREAM, 0);
	return 0;
}
