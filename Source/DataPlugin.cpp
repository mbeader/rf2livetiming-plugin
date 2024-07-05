#include "DataPlugin.hpp"          // corresponding header file
#include <math.h>               // for atan2, sqrt
#include <stdio.h>              // for sample output
#include <winsock.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define TIME_LENGTH 26
#define MAX_PACKET_LEN 32768
#define TCP_PACKET_LEN 512

// plugin information
unsigned g_uPluginID          = 0;
char     g_szPluginName[]     = "rf1livetiming";
unsigned g_uPluginVersion     = 001;
unsigned g_uPluginObjectCount = 1;
InternalsPluginInfo g_PluginInfo;

// interface to plugin information
extern "C" __declspec(dllexport) const char* __cdecl GetPluginName() { return g_szPluginName; }
extern "C" __declspec(dllexport) unsigned __cdecl GetPluginVersion() { return g_uPluginVersion; }
extern "C" __declspec(dllexport) unsigned __cdecl GetPluginObjectCount() { return g_uPluginObjectCount; }

// get the plugin-info object used to create the plugin.
extern "C" __declspec(dllexport) PluginObjectInfo* __cdecl GetPluginObjectInfo( const unsigned uIndex ) {
  switch(uIndex) {
    case 0:
      return  &g_PluginInfo;
    default:
      return 0;
  }
}

// InternalsPluginInfo class
InternalsPluginInfo::InternalsPluginInfo() {
  // put together a name for this plugin
  sprintf( m_szFullName, "%s - %s", g_szPluginName, InternalsPluginInfo::GetName() );
}

const char*    InternalsPluginInfo::GetName()     const { return ExampleInternalsPlugin::GetName(); }
const char*    InternalsPluginInfo::GetFullName() const { return m_szFullName; }
const char*    InternalsPluginInfo::GetDesc()     const { return "rf1livetiming"; }
const unsigned InternalsPluginInfo::GetType()     const { return ExampleInternalsPlugin::GetType(); }
const char*    InternalsPluginInfo::GetSubType()  const { return ExampleInternalsPlugin::GetSubType(); }
const unsigned InternalsPluginInfo::GetVersion()  const { return ExampleInternalsPlugin::GetVersion(); }
void*          InternalsPluginInfo::Create()      const { return new ExampleInternalsPlugin(); }

// InternalsPlugin class
const char ExampleInternalsPlugin::m_szName[] = "Data Plugin";
const char ExampleInternalsPlugin::m_szSubType[] = "Internals";
const unsigned ExampleInternalsPlugin::m_uID = 1;
const unsigned ExampleInternalsPlugin::m_uVersion = 3; // set to 3 for InternalsPluginV3 functionality and added graphical and vehicle info

PluginObjectInfo *ExampleInternalsPlugin::GetInfo() {
  return &g_PluginInfo;
}

void ExampleInternalsPlugin::log(const char *msg) {
	FILE *logFile;
	time_t curtime;
	struct tm loctime;
	char thetime[TIME_LENGTH];

	int err = fopen_s(&logFile, "UserData\\Log\\rf2livetiming.log", "a");
	if(err == 0) {
		curtime = time(NULL);
		int err2 = localtime_s(&loctime, &curtime);
		int err3 = asctime_s(thetime, TIME_LENGTH, &loctime);
		thetime[TIME_LENGTH - 2] = 0;
		fprintf(logFile, "[%s] %s\n", thetime, msg);
		fclose(logFile);
	}
}

void ExampleInternalsPlugin::Startup() {
	/*FILE *settings;
	struct hostent *ptrh;
	data_version = 2;

	// open socket
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		log("could not create datagram socket");
		return;
	}
	settings = fopen("DataSettings.ini", "r");
	if (settings != NULL) {
		fscanf(settings, "%s", hostname);
		ptrh = gethostbyname(hostname);
		fclose(settings);
	}
	else {
		ptrh = gethostbyname("localhost"); // Convert host name to equivalent IP address and copy to sad.
	}
	memset((char *)&sad, 0, sizeof(sad)); // clear sockaddr structure
	sad.sin_family = AF_INET;           // set family to Internet 
	sad.sin_port = htons((u_short) 6788);
	if (((char *)ptrh) == NULL) {
		log("invalid host name");
		return;
	}
	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);*/
	FILE *settings;
	data_version = 2;
	last_check = 0;
	char portstring[10];
	char iniip[16];
	int localhost;
	int send_results;
	int errcode;
	struct hostent *ptrh;

	log("starting plugin");

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if(s < 0) {
		log("could not create datagram socket");
		return;
	}
	int err = fopen_s(&settings, "rf1livetiming.ini", "r");
	if(err == 0) {
		log("reading settings");
		if(fscanf_s(settings, "USE LOCALHOST=\"%i\"\n", &localhost) != 1) {
			log("could not read localhost flag, using default: 1");
			localhost = 1;
		}
		else if(localhost)
			log("localhost");
		if(fscanf_s(settings, "DEST IP=\"%[^\"]\"\n", iniip, _countof(iniip)) != 1) {
			if(localhost)
				log("could not read IP, but using localhost");
			else
				log("could not read IP, using default: 127.0.0.1");
		}
		else if(!localhost)
			log(iniip);
		if(fscanf_s(settings, "DEST PORT=\"%i\"\n", &port) != 1) {
			log("could not read port, using default: 6789");
			port = 6789;
		}
		else {
			sprintf_s(portstring, "%i", port);
			log(portstring);
		}
		if(fscanf_s(settings, "SEND RESULTS=\"%i\"\n", &send_results) != 1) {
			log("could not read send results flag, using default: 0");
			send_results = 0;
		}
		else if(send_results)
			log("Will send results files");
		if(fscanf_s(settings, "SERVER NAME=\"%[^\"]\"\n", servername, _countof(servername)) != 1) {
			log("could not read server name, using default: rfactor server");
			strncpy(servername, "rfactor server\0", sizeof(servername));
		}
		else {
			log(servername);
		}
		if(fscanf_s(settings, "SERVER IP=\"%[^\"]\"\n", iniip, _countof(iniip)) != 1) {
			log("could not read server IP, using default: 0");
		}
		if(fscanf_s(settings, "SERVER PORT=\"%u\"", &serverport) != 1) {
			log("could not read server port, using default: 0");
			serverport = 0;
		}
		else {
			sprintf_s(portstring, "%u", serverport);
			log(portstring);
		}
		fclose(settings);

		/*if(localhost)
			errcode = getaddrinfo("localhost", NULL, &udp_hints, &udp_pResult);
		else
			errcode = getaddrinfo(iniip, NULL, &udp_hints, &udp_pResult);*/
		ptrh = gethostbyname("localhost");
	}
	else {
		log("could not read settings, using defaults: localhost:6789");
		//errcode = getaddrinfo("localhost", NULL, &udp_hints, &udp_pResult);
		ptrh = gethostbyname("localhost");
		port = 6789;
	}
	if(((char *)ptrh) == NULL) {
		log("invalid host name");
		return;
	}
	memset((char *)&udp_sad, 0, sizeof(udp_sad)); /* clear sockaddr structure */
	udp_sad.sin_family = AF_INET;           /* set family to Internet     */
	udp_sad.sin_port = htons((u_short)port);
	//udp_sad.sin_addr.S_un.S_addr = *((ULONG*)&(((sockaddr_in*)udp_pResult->ai_addr)->sin_addr));
	memcpy(&udp_sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	memset(empty, 0, sizeof(empty));
}

void ExampleInternalsPlugin::Shutdown() {
	if (s > 0) {
		closesocket(s);
		s = 0;
	}
}

void ExampleInternalsPlugin::StartSession() {
}

void ExampleInternalsPlugin::EndSession() {
}

void ExampleInternalsPlugin::EnterRealtime() {
	mET = 0.0f;
}

void ExampleInternalsPlugin::ExitRealtime() {
}

void ExampleInternalsPlugin::UpdateTelemetry(const TelemInfoV2 &info) {
}


void ExampleInternalsPlugin::UpdateScoring(const ScoringInfoV2 &info) {
	//log("starting update\n");
	StartStream();
	StreamData((char *)&type_scoring, sizeof(char));

	// multiplayer data added in newer plugin version
	/*StreamData((char *)&info.mServerPublicIP, sizeof(long));
	StreamData((char *)&info.mServerPort, sizeof(short));
	StreamString((char *)&info.mServerName, 32);
	StreamData((char *)&info.mMaxPlayers, sizeof(long));
	StreamData((char *)&info.mStartET, sizeof(float));*/
	StreamData((char *)&serverip, sizeof(long));
	StreamData((char *)&serverport, sizeof(short));
	StreamString((char *)&servername, 32);
	StreamData((char *)&empty, sizeof(long));
	StreamData((char *)&empty, sizeof(float));

	// session data (changes mostly with changing sessions)
	StreamString((char *)&info.mTrackName, 64);
	StreamData((char *)&info.mSession, sizeof(long));

	// event data (changes continuously)
	StreamData((char *)&info.mCurrentET, sizeof(float));
	StreamData((char *)&empty, sizeof(float));
	StreamData((char *)&info.mEndET, sizeof(float));
	StreamData((char *)&empty, sizeof(float));
	StreamData((char *)&info.mMaxLaps, sizeof(long));
	StreamData((char *)&info.mLapDist, sizeof(float));
	StreamData((char *)&empty, sizeof(float));
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
		VehicleScoringInfoV2 &vinfo = info.mVehicle[i];
		StreamData((char *)&vinfo.mPos.x, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mPos.z, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mPlace, sizeof(char));
		StreamData((char *)&vinfo.mLapDist, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mPathLateral, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		const float metersPerSec = sqrt((vinfo.mLocalVel.x * vinfo.mLocalVel.x) +
			(vinfo.mLocalVel.y * vinfo.mLocalVel.y) +
			(vinfo.mLocalVel.z * vinfo.mLocalVel.z));
		StreamData((char *)&metersPerSec, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamString((char *)&vinfo.mVehicleName, 64);
		StreamString((char *)&vinfo.mDriverName, 32);
		StreamString((char *)&vinfo.mVehicleClass, 32);
		StreamData((char *)&vinfo.mTotalLaps, sizeof(short));
		StreamData((char *)&vinfo.mBestSector1, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mBestSector2, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mBestLapTime, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mLastSector1, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mLastSector2, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mLastLapTime, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mCurSector1, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mCurSector2, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mTimeBehindLeader, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
		StreamData((char *)&vinfo.mLapsBehindLeader, sizeof(long));
		StreamData((char *)&vinfo.mTimeBehindNext, sizeof(float));
		StreamData((char *)&empty, sizeof(float));
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

void ExampleInternalsPlugin::StartStream() {
	data_packet = 0;
	data_sequence++;
	data[0] = data_version;
	data[1] = data_packet;
	memcpy(&data[2], &data_sequence, sizeof(short));
	data_offset = 4;
}

void ExampleInternalsPlugin::StreamData(char *data_ptr, int length) {
	int i;

	for (i = 0; i < length; i++) {
		if (data_offset + i == MAX_PACKET_LEN) {
			sendto(s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
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

void ExampleInternalsPlugin::StreamVarString(char *data_ptr) {
	int i = 0;
	while (data_ptr[i] != 0) {
		i++;
	}
	StreamData((char *)&i, sizeof(int));
	StreamString(data_ptr, i);
}

void ExampleInternalsPlugin::StreamString(char *data_ptr, int length) {
	int i;

	for (i = 0; i < length; i++) {
		if (data_offset + i == MAX_PACKET_LEN) {
			sendto(s, data, MAX_PACKET_LEN, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
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

void ExampleInternalsPlugin::EndStream() {
	if (data_offset > 4) {
		sendto(s, data, data_offset, 0, (struct sockaddr *) &udp_sad, sizeof(struct sockaddr));
	}
}
