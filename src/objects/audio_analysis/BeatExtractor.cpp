/*==============================================================================

    ofxVisualProgramming: A visual programming patching environment for OF

    Copyright (c) 2018 Emanuele Mazza aka n3m3da <emanuelemazza@d3cod3.org>

    ofxVisualProgramming is distributed under the MIT License.
    This gives everyone the freedoms to use ofxVisualProgramming in any context:
    commercial or non-commercial, public or private, open or closed source.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    See https://github.com/d3cod3/ofxVisualProgramming for documentation

==============================================================================*/

#include "BeatExtractor.h"

//--------------------------------------------------------------
BeatExtractor::BeatExtractor() : PatchObject(){

    this->numInlets  = 1;
    this->numOutlets = 1;

    _inletParams[0] = new vector<float>();  // RAW Data

    _outletParams[0] = new float(); // beat
    *(float *)&_outletParams[0] = 0.0f;

    this->initInletsState();

    isNewConnection   = false;
    isConnectionRight = false;

}

//--------------------------------------------------------------
void BeatExtractor::newObject(){
    this->setName("beat extractor");
    this->addInlet(VP_LINK_ARRAY,"data");
    this->addOutlet(VP_LINK_NUMERIC,"beat");
}

//--------------------------------------------------------------
void BeatExtractor::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    
}

//--------------------------------------------------------------
void BeatExtractor::updateObjectContent(map<int,PatchObject*> &patchObjects, ofxThreadedFileDialog &fd){

    if(this->inletsConnected[0]){
        if(!isNewConnection){
            isNewConnection = true;
            for(map<int,PatchObject*>::iterator it = patchObjects.begin(); it != patchObjects.end(); it++ ){
                if(patchObjects[it->first] != nullptr && it->first != this->getId() && !patchObjects[it->first]->getWillErase()){
                    for(int o=0;o<static_cast<int>(it->second->outPut.size());o++){
                        if(!it->second->outPut[o]->isDisabled && it->second->outPut[o]->toObjectID == this->getId()){
                            if(it->second->getName() == "audio analyzer"){
                                isConnectionRight = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }else{
        isNewConnection = false;
        isConnectionRight = false;
    }

    if(this->inletsConnected[0] && !static_cast<vector<float> *>(_inletParams[0])->empty() && isConnectionRight){
        *(float *)&_outletParams[0] = static_cast<vector<float> *>(_inletParams[0])->back();
    }else if(this->inletsConnected[0] && !isConnectionRight){
        ofLog(OF_LOG_ERROR,"%s --> This object can receive data from audio analyzer object ONLY! Just reconnect it right!",this->getName().c_str());
    }

}

//--------------------------------------------------------------
void BeatExtractor::drawObjectContent(ofxFontStash *font){
    ofSetColor(255);
    ofEnableAlphaBlending();
    if(*(float *)&_outletParams[0] > 0){
        ofSetColor(250,250,5);
        ofDrawRectangle(0,0,this->width,this->height);
    }
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void BeatExtractor::removeObjectContent(){
    
}
