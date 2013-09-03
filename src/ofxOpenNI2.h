//
//  ofxOpenNI2.h
//
//  Created by Niisato Hirotaka on 13/02/11.
//

#ifndef emptyExample_ofxOpenNI2_h
#define emptyExample_ofxOpenNI2_h

#include "NiTE.h"
#include "ofThread.h"
#include "ofTexture.h"
#include "ofPixels.h"
#include <map>
#include <vector>

#define MAX_DEPTH 10000
#define MAX_USERS 10

class ofxOpenNI2 : public ofThread {
private:
    bool useDepth, useColor, useTracker;
    int colorHeight, colorWidth, depthHeight, depthWidth;
    int width, height;
    
    openni::Device              device;
    
    nite::UserTracker           userTracker;
    nite::UserTrackerFrameRef   userTrackerFrame;
    openni::VideoFrameRef       userTrackerDepthFrame;
    ofPixels                    userTrackerPixels;
    ofTexture                   userTrackerTexture;
    
    map <int, nite::UserData>   trackedUsers;
    vector <int>                trackedUserIds;
    NiteUserData                defUserData;

    openni::VideoFrameRef       depthFrame;
    openni::VideoStream         depthStream;
    ofPixels                    depthPixels;
    ofTexture                   depthTexture;
    
    openni::VideoFrameRef       colorFrame;    
    openni::VideoStream         colorStream;
    ofPixels                    colorPixels;
    ofTexture                   colorTexture;
    
    float   depthHist[MAX_DEPTH];
    void calculateHistogram(float* histogram, int histogramSize, const openni::VideoFrameRef& frame);
    
protected:
    void threadedFunction();
    void checkStream();
    
public:
    ofxOpenNI2();
    ~ofxOpenNI2();
    
    void setup();
    void start();
    void stop();
    
    void initDepth();
    void initImage();
    void initTracker();
    
    ofPixels        getColorPixels();
    ofTexture       getColorTexture();
    
    ofPixels        getDepthPixels();
    ofTexture       getDepthTexture();
    float           getDepth(int x, int y);
    
    int             getTrackedUsers();
    nite::UserData  getUserData(int id);
    ofPixels        getTrackerPixels();
    ofTexture       getTrackerTexture();

	ofMutex			colorMutex;
	ofMutex			depthMutex;
	ofMutex			trackerMutex;
};

class ofxOpenNIScopedLock {
public:
    ofxOpenNIScopedLock(ofMutex & _mutex): mutex(_mutex){
        mutex.lock();
    };
    ~ofxOpenNIScopedLock(){
        mutex.unlock();
    };
    ofMutex & mutex;
};

#endif
