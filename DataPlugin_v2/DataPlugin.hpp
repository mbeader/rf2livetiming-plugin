#pragma once

#include "InternalsPlugin.hpp"

class DataPlugin : public InternalsPluginV06
{
public:
	DataPlugin();
	~DataPlugin();

	// These are the functions derived from base class InternalsPlugin
	// that can be implemented.
	void Startup(long version);  // game startup
	void Shutdown();               // game shutdown

	void Load();                   // scene/track load
	void Unload();                 // scene/track unload

	void EnterRealtime();          // entering realtime
	void ExitRealtime();           // exiting realtime

	void StartSession();           // session has started
	void EndSession();             // session has ended

	// GAME OUTPUT
	long WantsTelemetryUpdates() { return(2); } // CHANGE TO 1 TO ENABLE TELEMETRY EXAMPLE!
	void UpdateTelemetry(const TelemInfoV01 &info);

	bool WantsGraphicsUpdates() { return(false); } // CHANGE TO TRUE TO ENABLE GRAPHICS EXAMPLE!
	// EXTENDED GAME OUTPUT
	virtual void UpdateGraphics(const GraphicsInfoV02 &info)          {} // update plugin with extended graphics info

	// GAME INPUT
	bool HasHardwareInputs() { return(false); } // CHANGE TO TRUE TO ENABLE HARDWARE EXAMPLE!
	void UpdateHardware(const float fDT) { mET += fDT; } // update the hardware with the time between frames
	void EnableHardware() { mEnabled = true; }             // message from game to enable hardware
	void DisableHardware() { mEnabled = false; }           // message from game to disable hardware

	// See if the plugin wants to take over a hardware control.  If the plugin takes over the
	// control, this method returns true and sets the value of the float pointed to by the
	// second arg.  Otherwise, it returns false and leaves the float unmodified.
	bool CheckHWControl(const char * const controlName, float &fRetVal);

	bool ForceFeedback(float &forceValue);  // SEE FUNCTION BODY TO ENABLE FORCE EXAMPLE

	// SCORING OUTPUT
	bool WantsScoringUpdates() { return(true); } // CHANGE TO TRUE TO ENABLE SCORING EXAMPLE!
	void UpdateScoring(const ScoringInfoV01 &info);

	// COMMENTARY INPUT
	virtual bool RequestCommentary(CommentaryRequestInfoV01 &info) { return(false); } // to use our commentary event system, fill in data and return true

	// MESSAGE BOX INPUT
	bool WantsToDisplayMessage(MessageInfoV01 &msgInfo);

	// CAMERA CONTROL
	unsigned char WantsToViewVehicle(CameraControlInfoV01 &camControl);

	// ERROR FEEDBACK
	virtual void Error(const char * const msg); // Called with explanation message if there was some sort of error in a plugin callback

	// VIDEO EXPORT (sorry, no example code at this time)
	virtual bool WantsVideoOutput() { return(false); }         // whether we want to export video
	virtual bool VideoOpen(const char * const szFilename, float fQuality, unsigned short usFPS, unsigned long fBPS,
		unsigned short usWidth, unsigned short usHeight, char *cpCodec = NULL) {
		return(false);
	} // open video output file
	virtual void VideoClose() {}                                 // close video output file
	virtual void VideoWriteAudio(const short *pAudio, unsigned int uNumFrames) {} // write some audio info
	virtual void VideoWriteImage(const unsigned char *pImage) {} // write video image

private:
	// marrs:
	float mET;  // event time
	bool mEnabled; // needed for the hardware example
	// constant types
	static const char type_telemetry = 1;
	static const char type_scoring = 2;

	// marrs:
	void StartStream();
	void StreamData(char *data_ptr, int length);
	void StreamString(char *data_ptr, int length);
	void StreamVarString(char *data_ptr);
	void EndStream();
	void log(const char *msg);

	SOCKET s; // socket to send data to
	struct sockaddr_in sad;
	char data[32768];
	int data_offset;
	byte data_version;
	byte data_packet;
	short data_sequence;
	char hostname[256];
	int port;
};

