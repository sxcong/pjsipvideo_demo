// RTSP.cpp: implementation of the CRTSP class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <stdint.h>
#include "RTSPStream.h"
#include "rtspAPI.h"
#include <math.h>
#include <string>
using namespace std;

#include "src/pjsua_app.h"

//for gettimeofday
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRTSPSession::CRTSPSession()
{
	m_rtspClient = NULL;
	m_running = false;
	m_nStatus = 0;
	eventLoopWatchVariable = 0;
	m_nID = 0;
	m_pCB = NULL;
}

CRTSPSession::~CRTSPSession()
{

}

int CRTSPSession::CheckStatus()
{
	return m_nStatus;
}

int CRTSPSession::startRTSPClient(char const* progName, char const* rtspURL, int debugLevel)
{
	if (m_nStatus != 0)
		return 1;

	m_progName = progName;
	m_rtspUrl = rtspURL;
	m_debugLevel = debugLevel;
	eventLoopWatchVariable = 0;

	

	int r = pthread_create (&tid, NULL, rtsp_thread_fun, this);
	if (r)
	{
		perror ("pthread_create()");
		//retval = -1;
		//goto terminate;
		return -1;
	}

	return 0;
}

int CRTSPSession::stopRTSPClient()
{
	if (m_nStatus == 2)
	{
	}
	else
	{
		eventLoopWatchVariable = 1;
	}
	m_running = false;
	return 0;
}

void *CRTSPSession::rtsp_thread_fun(void *param)
{
	CRTSPSession *pThis = (CRTSPSession*)param;
	pThis->rtsp_fun ();
	return NULL;
}

void CRTSPSession::rtsp_fun()
{
	//::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	if (openURL(*env, m_progName.c_str(), m_rtspUrl.c_str(), m_debugLevel) == 0)
	{
		m_nStatus = 1;
		env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
		
		m_running = false;
		eventLoopWatchVariable = 0;
		
		if (m_rtspClient)
		{
			shutdownStream(m_rtspClient,0);
		}
		m_rtspClient = NULL;
	}
	
	env->reclaim(); 

	env = NULL;
	delete scheduler; 
	scheduler = NULL;
	m_nStatus = 2;
}


int CRTSPSession::openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel)
{
	m_rtspClient = ourRTSPClient::createNew(env, rtspURL, debugLevel, progName);
	if (m_rtspClient == NULL) 
	{
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return -1;
	}

	((ourRTSPClient*)m_rtspClient)->m_nID = m_nID;

	m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
	return 0;
}

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL)
	{
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } 
	else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient, 1);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient)
 {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession
	  << "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession
	<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

	//暂时只处理视频
	string name = scs.subsession->mediumName();
	string name2 = scs.subsession->codecName();

	if (strcmp(scs.subsession->mediumName(), "video") == 0 &&
		(strcmp(scs.subsession->codecName(), "H264") == 0)) 
	{
		DummySink* pVideoSink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
		pVideoSink->m_nIndex = ((ourRTSPClient*)rtspClient)->m_nID;

		string name = pVideoSink->name();
	    pVideoSink->sps = scs.subsession->fmtp_spropparametersets();

        //env <<*rtspClient<<"sps=" << pVideoSink->sps.c_str()<<endl;

		scs.subsession->sink = pVideoSink;
	}

      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient, 2);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient, 3);
}

void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient, 4);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode)
 {
	ourRTSPClient* rtsp = (ourRTSPClient*)rtspClient;
	if (exitCode != 0)
	{
		//eventLoopWatchVariable = 1;
		//int nID = rtsp->m_nID;
		IPNC_CloseStream(rtsp->m_nID);
		return;
	}

  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);

  
  rtsp = NULL;

  rtspClient = NULL;
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  //if (--rtspClientCount == 0)
  {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    //exit(exitCode);
	 //eventLoopWatchVariable = 1;
	//CXAgent::Instance().do_exit();
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
	m_nStatus = 0;
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() 
{
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:\\

//1280*760*3
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 2764800
uint8_t* buff = NULL;

int starttime;

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];

   buff = new uint8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
   sPropRecords = NULL;
   m_nIndex = -1;
  //pf = fopen("/home/temp/1.h264", "wb");
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
  delete []buff;
  if (sPropRecords)
	delete[] sPropRecords;

}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

int Width = 0;
int Height = 0;
unsigned numSPropRecords;



bool h264_decode_seq_parameter_set(uint8_t * buf, uint32_t nLen, int &Width, int &Height);

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) 
{

/*
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
 // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) 
	  envir() << " (with " << numTruncatedBytes << " bytes truncated)";

  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;

  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) 
  {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }

  string name = this->name();
  name = fStreamId;

#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
  */

  static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

  if (sPropRecords == NULL)
  {
      sPropRecords = parseSPropParameterSets(sps.c_str(), numSPropRecords);
      char* filename = "test.264";
      FILE* pfSPS = fopen(filename, "wb");
      envir() <<sps.c_str()<<"!\r\n";

	  for (unsigned i = 0; i < numSPropRecords; ++i) 
	  {
          if (pfSPS)
		  {
              fwrite(start_code, 1, 4, pfSPS );
              fwrite(sPropRecords[i].sPropBytes, 1, sPropRecords[i].sPropLength, pfSPS);
              //CBuffMgr::Instance().putBuffer(m_nIndex, sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength, 0);
		  }
	  }
      fclose(pfSPS);

      gettimeofday(&start_time, NULL);

	  //CXAgent::Instance().SetSPS(filename);
  }
  
  memset(buff, '0', DUMMY_SINK_RECEIVE_BUFFER_SIZE);
  memcpy(buff, start_code, 4);
  memcpy(buff+4, fReceiveBuffer, frameSize);
  
  
  // presentationTime.tv_sec (unsigned)presentationTime.tv_usec
  struct timeval cur;
  gettimeofday(&cur, NULL);
  int timestamp = (cur.tv_sec-start_time.tv_sec)*1000 + (cur.tv_usec - start_time.tv_usec)/1000;


  //CXAgent::Instance().SetData(buff, frameSize+4, timestamp);

  //check width and height

 /* bool ret = h264_decode_seq_parameter_set(fReceiveBuffer, frameSize, Width, Height);
  if (ret)
  {
  }*/

  
    if (m_nIndex >= 0)
    {
		pjcall_thread_register();
        if (rtspAPI::Instance().m_rtsp[m_nIndex]->m_pCB)
        {
            rtspAPI::Instance().m_rtsp[m_nIndex]->m_pCB->rtspDataCB(buff,frameSize+4);
            int result = rtspAPI::Instance().m_rtsp[m_nIndex]->m_pDecode->decode_rtsp_frame(buff, frameSize+4, false);
        }
    }
	
    continuePlaying();
}

Boolean DummySink::continuePlaying() 
{
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}


//参考: http://blog.csdn.net/f6991/article/details/7852406

