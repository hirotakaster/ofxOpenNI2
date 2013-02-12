//
//  ofxOpenNI2.cpp
//
//  Created by Niisato Hirotaka on 13/02/10.
//

#include <iostream>
#include "ofxOpenNI2.h"

ofxOpenNI2::ofxOpenNI2() {
    useDepth = useColor = useTracker = false;
    defUserData.id = -1;
}

ofxOpenNI2::~ofxOpenNI2() {
    stop();
}

void ofxOpenNI2::setup() {
    openni::Status rc = openni::OpenNI::initialize();
    if (rc != openni::STATUS_OK) {
        cout << "Failed to initialize OpenNI:" << openni::OpenNI::getExtendedError() << endl;
        return rc;
    }
    
    const char* deviceUri = openni::ANY_DEVICE;
    
    rc = device.open(deviceUri);
    if (rc != openni::STATUS_OK) {
        cout << "Failed to open device:" << openni::OpenNI::getExtendedError() << endl;
        return rc;
    }
    
    device.setDepthColorSyncEnabled(true);
}

void ofxOpenNI2::threadedFunction() {
    while (this->isThreadRunning()) {
        this->checkStream();
    }
}

void ofxOpenNI2::checkStream() {
    ofxOpenNIScopedLock scopedLock(mutex);
    
    // check depth stream
    if (useDepth) {
        depthStream.readFrame(&depthFrame);
        calculateHistogram(depthHist, MAX_DEPTH, depthFrame);

        const openni::DepthPixel* depthRow = (const openni::DepthPixel*)depthFrame.getData();
        for(int y = 0; y < height; y++){
            for(int x = 0; x < width; x++, depthRow++){
                depthPixels.setColor(x, y, *depthRow);
            }
        }

        const openni::DepthPixel* depth = (const openni::DepthPixel*)depthFrame.getData();
        for (int y = depthFrame.getCropOriginY(); y < depthFrame.getHeight() + depthFrame.getCropOriginY(); y++){
            unsigned char * texture = depthPixels.getPixels() + y * depthFrame.getWidth() * 4 + depthFrame.getCropOriginX() * 4;
            for (int x = 0; x < depthPixels.getWidth(); x++, depth++, texture += 4) {
                int nHistValue = depthHist[*depth];
                if (*depth != 0){
                    texture[0] = nHistValue;
                    texture[1] = nHistValue;
                    texture[2] = 0;
                }
            }
        }
    }
    
    // check color stream    
    if (useColor) {
        colorStream.readFrame(&colorFrame);
        colorPixels.setFromPixels((unsigned char *)colorFrame.getData(), colorFrame.getWidth(), colorFrame.getHeight(), OF_IMAGE_COLOR);
    }

    // check user tracker
    if (useTracker) {
        nite::Status niteRc = userTracker.readFrame(&userTrackerFrame);
        if (niteRc != nite::STATUS_OK) {
            cout << "Get next frame failed" << endl;
        } else {
            const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();

            // clear tracked data
            trackedUserIds.clear();
            trackedUsers.clear();
            
            for (int i = 0; i < users.getSize(); ++i) {
                const nite::UserData& user = users[i];
                if (user.isNew()) {
                    userTracker.startSkeletonTracking(user.getId());
                }
                if (!user.isLost()) {
                    trackedUserIds.push_back(user.getId());
                    trackedUsers[(int)user.getId()] = user;
                }
            }
            
            
            // tracked user images
            userTrackerDepthFrame = userTrackerFrame.getDepthFrame();
            calculateHistogram(depthHist, MAX_DEPTH, userTrackerDepthFrame);

            const openni::DepthPixel* depthRow = (const openni::DepthPixel*)userTrackerDepthFrame.getData();
            for(int y = 0; y < height; y++){
                for(int x = 0; x < width; x++, depthRow++){
                    userTrackerPixels.setColor(x, y, *depthRow);
                }
            }

            const openni::DepthPixel* depth = (const openni::DepthPixel*)userTrackerDepthFrame.getData();
            const nite::UserId* pLabels = userTrackerFrame.getUserMap().getPixels();
            for (int y = userTrackerDepthFrame.getCropOriginY(); y < userTrackerDepthFrame.getHeight() + userTrackerDepthFrame.getCropOriginY(); y++){
                unsigned char * texture = userTrackerPixels.getPixels() + y * userTrackerDepthFrame.getWidth() * 4 + userTrackerDepthFrame.getCropOriginX() * 4;
                for (int x = 0; x < userTrackerPixels.getWidth(); x++, depth++, texture += 4, ++pLabels) {
                    int nHistValue = depthHist[*depth];
                    if (*pLabels != 0) {
                        // sample
                        texture[0] = (*pLabels % 2 == 1 ? 255 : 0);
                        texture[1] = (*pLabels % 3 == 1 ? 255 : 0);
                        texture[2] = (*pLabels % 3 == 2 ? 255 : 0);
                    } else if (*depth != 0){
                        texture[0] = nHistValue;
                        texture[1] = nHistValue;
                        texture[2] = 0;
                    }
                }
            }
        }
    }
}

void ofxOpenNI2::start() {
    if (depthWidth == colorWidth && depthHeight == colorHeight) {
        width = depthWidth;
        height = depthHeight;
    } else {
        return;
    }
    
    if (useDepth) {
        depthPixels.allocate(width, height, OF_IMAGE_COLOR_ALPHA);
        depthTexture.allocate(width, height, GL_RGBA);
    }
    
    if (useColor) {
        colorPixels.allocate(width, height, OF_IMAGE_COLOR);
        colorTexture.allocate(width, height, GL_RGB);
    }
    
    if (useTracker) {
        userTrackerPixels.allocate(width, height, OF_IMAGE_COLOR_ALPHA);
        userTrackerTexture.allocate(width, height, GL_RGBA);
    }
    startThread();
}

void ofxOpenNI2::stop() {
    stopThread();
}

void ofxOpenNI2::initDepth() {
    useDepth = true;
    
    openni::Status rc = depthStream.create(device, openni::SENSOR_DEPTH);
    if (rc == openni::STATUS_OK) {
        rc = depthStream.start();
        if (rc != openni::STATUS_OK) {
            cout << "SimpleViewer: Couldn't start depth stream:" << openni::OpenNI::getExtendedError() << endl;
            depthStream.destroy();
        }
    } else {
        cout << "SimpleViewer: Couldn't find depth stream:" << openni::OpenNI::getExtendedError() << endl;
    }

    depthWidth = depthStream.getVideoMode().getResolutionX();
    depthHeight = depthStream.getVideoMode().getResolutionY();

}

void ofxOpenNI2::initImage() {
    useColor = true;
    
    openni::Status rc = colorStream.create(device, openni::SENSOR_COLOR);
    if (rc == openni::STATUS_OK) {
        rc = colorStream.start();
        if (rc != openni::STATUS_OK) {
            cout << "SimpleViewer: Couldn't start color stream:" << openni::OpenNI::getExtendedError() << endl;
            colorStream.destroy();
        }
    } else {
        cout << "SimpleViewer: Couldn't find color stream:" << openni::OpenNI::getExtendedError() << endl;
    }
    
    colorWidth = colorStream.getVideoMode().getResolutionX();
    colorHeight = colorStream.getVideoMode().getResolutionY();
}


void ofxOpenNI2::calculateHistogram(float* histogram, int histogramSize, const openni::VideoFrameRef& frame) {
    const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
    memset(histogram, 0, histogramSize*sizeof(float));
    int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
    int height = frame.getHeight();
    int width = frame.getWidth();
    
    unsigned int nNumberOfPoints = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x, ++pDepth) {
            if (*pDepth != 0) {
                histogram[*pDepth]++;
                nNumberOfPoints++;
            }
        }
        pDepth += restOfRow;
    }
    
    for (int nIndex=1; nIndex<histogramSize; nIndex++) {
        histogram[nIndex] += histogram[nIndex-1];
    }
    
    if (nNumberOfPoints) {
        for (int nIndex=1; nIndex<histogramSize; nIndex++) {
            histogram[nIndex] = (256 * (1.0f - (histogram[nIndex] / nNumberOfPoints)));
        }
    }
}


void ofxOpenNI2::initTracker() {
    useTracker = true;
    nite::NiTE::initialize();
    if (userTracker.create(&device) != nite::STATUS_OK) {
        return openni::STATUS_ERROR;
    }
}

ofPixels ofxOpenNI2::getColorPixels() {
    ofxOpenNIScopedLock scopedLock(mutex);
    return colorPixels;
}

ofTexture ofxOpenNI2::getColorTexture() {
    ofxOpenNIScopedLock scopedLock(mutex);
    colorTexture.loadData(colorPixels);
    return colorTexture;
}

ofPixels ofxOpenNI2::getDepthPixels() {
    ofxOpenNIScopedLock scopedLock(mutex);
    return depthPixels;
}

ofTexture ofxOpenNI2::getDepthTexture() {
    ofxOpenNIScopedLock scopedLock(mutex);
    depthTexture.loadData(depthPixels.getPixels(), width, height, GL_RGBA);
    return depthTexture;
}

ofPixels ofxOpenNI2::getTrackerPixels() {
    ofxOpenNIScopedLock scopedLock(mutex);
    return userTrackerPixels;
}

ofTexture ofxOpenNI2::getTrackerTexture() {
    ofxOpenNIScopedLock scopedLock(mutex);
    depthTexture.loadData(userTrackerPixels.getPixels(), width, height, GL_RGBA);
    return depthTexture;
}


float ofxOpenNI2::getDepth(int x, int y) {
    ofxOpenNIScopedLock scopedLock(mutex);
    if (x > width || y > height || x < 0 || y < 0) return 0;
    return depthPixels[depthPixels.getPixelIndex(x, y)];
}

int ofxOpenNI2::getTrackedUsers() {
    ofxOpenNIScopedLock scopedLock(mutex);
    return trackedUserIds.size();
}

nite::UserData ofxOpenNI2::getUserData(int id) {
    ofxOpenNIScopedLock scopedLock(mutex);
    if (id >= trackedUserIds.size()) return (nite::UserData &)defUserData;
    return trackedUsers[trackedUserIds[id]];
}
