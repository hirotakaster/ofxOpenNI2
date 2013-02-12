#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    ofSetBackgroundAuto(false);
    
    ofBackground(255, 255, 255);
    ofSetFrameRate(60);
    ofEnableAlphaBlending();
    ofSetCircleResolution(64);
    
    openNI2.setup();
    openNI2.initDepth();
    openNI2.initImage();
    openNI2.initTracker();
    openNI2.start();
}

//--------------------------------------------------------------
void testApp::update(){
}

//--------------------------------------------------------------
void testApp::draw(){
    ofTexture tex1 = openNI2.getDepthTexture();
    tex1.draw(100, 10);
    
    ofTexture tex2 = openNI2.getColorTexture();
    tex2.draw(100, 300);

    ofTexture tex3 = openNI2.getTrackerTexture();
    tex3.draw(560, 10);
    
    cout << openNI2.getTrackedUsers() << endl;
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}