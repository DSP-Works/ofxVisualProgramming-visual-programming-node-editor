/*==============================================================================

    ofxVisualProgramming: A visual programming patching environment for OF

    Copyright (c) 2019 Emanuele Mazza aka n3m3da <emanuelemazza@d3cod3.org>

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

#include "pdspADSR.h"

//--------------------------------------------------------------
pdspADSR::pdspADSR() : PatchObject(){

    this->numInlets  = 3;
    this->numOutlets = 1;

    _inletParams[0] = new ofSoundBuffer(); // audio input

    _inletParams[1] = new float();          // bang
    *(float *)&_inletParams[1] = 0.0f;
    _inletParams[2] = new float();          // duration
    *(float *)&_inletParams[2] = 0.0f;

    _outletParams[0] = new ofSoundBuffer(); // audio output

    this->initInletsState();

    this->width *= 2;

    isGUIObject             = true;
    this->isOverGUI         = true;

    isAudioINObject         = true;
    isAudioOUTObject        = true;
    isPDSPPatchableObject   = true;

    loaded                  = false;

    attackDuration          = 0.05f;
    decayDuration           = 0.1f;
    sustainDuration         = 0.45f;
    releaseDuration         = 0.4f;

    envelopeDuration        = 100;

    rect.set(120,this->headerHeight+10,this->width-130,this->height-this->headerHeight-40);

}

//--------------------------------------------------------------
void pdspADSR::newObject(){
    this->setName("ADSR envelope");
    this->addInlet(VP_LINK_AUDIO,"signal");
    this->addInlet(VP_LINK_NUMERIC,"bang");
    this->addInlet(VP_LINK_NUMERIC,"duration");
    this->addOutlet(VP_LINK_AUDIO,"envelopedSignal");

    this->setCustomVar(static_cast<float>(envelopeDuration),"DURATION");
    this->setCustomVar(1.0f,"ATTACK_CURVE");
    this->setCustomVar(1.0f,"RELEASE_CURVE");

    this->setCustomVar(120.0f,"ATTACK_X");
    this->setCustomVar(this->headerHeight+10,"ATTACK_Y");
    this->setCustomVar(120.0f+(this->width-130)/3,"DECAY_X");
    this->setCustomVar(this->headerHeight+40,"DECAY_Y");
    this->setCustomVar(120.0f+(this->width-130)/2,"SUSTAIN_X");
    this->setCustomVar(this->headerHeight+40,"SUSTAIN_Y");
    this->setCustomVar(120.0f+(this->width-130),"RELEASE_X");
    this->setCustomVar(this->headerHeight+10+(this->height-this->headerHeight-40),"RELEASE_Y");
}

//--------------------------------------------------------------
void pdspADSR::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    loadAudioSettings();

    controlPoints.push_back(DraggableVertex(this->getCustomVar("ATTACK_X"),this->getCustomVar("ATTACK_Y"))); // A
    controlPoints.push_back(DraggableVertex(this->getCustomVar("DECAY_X"),this->getCustomVar("DECAY_Y"))); // D
    controlPoints.push_back(DraggableVertex(this->getCustomVar("SUSTAIN_X"),this->getCustomVar("SUSTAIN_Y"))); // S
    controlPoints.push_back(DraggableVertex(this->getCustomVar("RELEASE_X"),this->getCustomVar("RELEASE_Y"))); // R

    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setAutoDraw(false);
    gui->setUseCustomMouse(true);
    gui->setWidth(this->width);
    gui->onTextInputEvent(this,&pdspADSR::onTextInputEvent);
    gui->onSliderEvent(this, &pdspADSR::onSliderEvent);

    header = gui->addHeader("CONFIG",false);
    header->setUseCustomMouse(true);
    header->setCollapsable(true);
    duration = gui->addTextInput("DURATION (MS)","");
    duration->setUseCustomMouse(true);
    duration->setText(ofToString(this->getCustomVar("DURATION")));
    attackHardness = gui->addSlider("Attack Curve", 0,1,this->getCustomVar("ATTACK_CURVE"));
    attackHardness->setUseCustomMouse(true);
    releaseHardness = gui->addSlider("Release Curve", 0,1,this->getCustomVar("RELEASE_CURVE"));
    releaseHardness->setUseCustomMouse(true);

    gui->setPosition(0,this->height - header->getHeight());
    gui->collapse();
    header->setIsCollapsed(true);

}

//--------------------------------------------------------------
void pdspADSR::setupAudioOutObjectContent(pdsp::Engine &engine){

    gate_ctrl.out_trig() >> env;
    env >> amp.in_mod();

    this->pdspIn[0] >> amp >> this->pdspOut[0];
    this->pdspIn[0] >> amp >> scope >> engine.blackhole();

}

//--------------------------------------------------------------
void pdspADSR::updateObjectContent(map<int,PatchObject*> &patchObjects, ofxThreadedFileDialog &fd){
    gui->update();
    header->update();
    duration->update();
    attackHardness->update();
    releaseHardness->update();

    attackDuration          = (controlPoints.at(0).x-rect.x)/rect.width;
    decayDuration           = (((controlPoints.at(1).x-rect.x)/rect.width * 100)-((controlPoints.at(0).x-rect.x)/rect.width * 100))/100;
    sustainDuration         = (((controlPoints.at(2).x-rect.x)/rect.width * 100)-((controlPoints.at(1).x-rect.x)/rect.width * 100))/100;
    releaseDuration         = (((controlPoints.at(3).x-rect.x)/rect.width * 100)-((controlPoints.at(2).x-rect.x)/rect.width * 100))/100;

    env.set(attackDuration*this->getCustomVar("DURATION"),decayDuration*this->getCustomVar("DURATION"),0.5f,releaseDuration*this->getCustomVar("DURATION"));

    // bang --> trigger envelope
    if(this->inletsConnected[1]){
        if(*(float *)&_inletParams[1] == 1.0f){
            gate_ctrl.trigger(ofClamp(*(float *)&_inletParams[1],0.0f,1.0f));
        }else{
            gate_ctrl.off();
        }
    }else{
        gate_ctrl.off();
    }

    // duration
    if(this->inletsConnected[2]){
        duration->setText(ofToString(static_cast<int>(floor(abs(*(float *)&_inletParams[2])))));
        this->setCustomVar(*(float *)&_inletParams[2],"DURATION");
    }

    if(!loaded){
        loaded = true;
        duration->setText(ofToString(static_cast<int>(floor(this->getCustomVar("DURATION")))));
        attackHardness->setValue(this->getCustomVar("ATTACK_CURVE"));
        releaseHardness->setValue(this->getCustomVar("RELEASE_CURVE"));
    }
}

//--------------------------------------------------------------
void pdspADSR::drawObjectContent(ofxFontStash *font){
    ofSetColor(255);
    ofEnableAlphaBlending();
    ofSetColor(255,255,120);
    for(size_t i=0;i<controlPoints.size();i++){
        if(controlPoints.at(i).bOver){
            ofDrawCircle(controlPoints.at(i).x,controlPoints.at(i).y,6);
        }else{
            ofDrawCircle(controlPoints.at(i).x,controlPoints.at(i).y,4);
        }

    }
    ofDrawLine(rect.getBottomLeft().x,rect.getBottomLeft().y,controlPoints.at(0).x,controlPoints.at(0).y);
    ofDrawLine(controlPoints.at(0).x,controlPoints.at(0).y,controlPoints.at(1).x,controlPoints.at(1).y);
    ofDrawLine(controlPoints.at(1).x,controlPoints.at(1).y,controlPoints.at(2).x,controlPoints.at(2).y);
    ofDrawLine(controlPoints.at(2).x,controlPoints.at(2).y,controlPoints.at(3).x,controlPoints.at(3).y);

    ofSetColor(160,160,160);
    font->draw("A:",this->fontSize,rect.getBottomLeft().x+6,rect.getBottomLeft().y+8);
    font->draw("D:",this->fontSize,rect.getCenter().x-50,rect.getBottomLeft().y+8);
    font->draw("S:",this->fontSize,rect.getCenter().x-4,rect.getBottomLeft().y+8);
    font->draw("R:",this->fontSize,rect.getBottomRight().x-60,rect.getBottomLeft().y+8);
    ofSetColor(255,255,120);
    string tempStr = ofToString(attackDuration * 100,0)+"%";
    font->draw(tempStr,this->fontSize,rect.getBottomLeft().x+20,rect.getBottomLeft().y+8);
    tempStr = ofToString(decayDuration * 100,0)+"%";
    font->draw(tempStr,this->fontSize,rect.getCenter().x-36,rect.getBottomLeft().y+8);
    tempStr = ofToString(sustainDuration * 100,0)+"%";
    font->draw(tempStr,this->fontSize,rect.getCenter().x+6,rect.getBottomLeft().y+8);
    tempStr = ofToString(releaseDuration * 100,0)+"%";
    font->draw(tempStr,this->fontSize,rect.getBottomRight().x-46,rect.getBottomLeft().y+8);

    gui->draw();
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void pdspADSR::removeObjectContent(){
    for(map<int,pdsp::PatchNode>::iterator it = this->pdspIn.begin(); it != this->pdspIn.end(); it++ ){
        it->second.disconnectAll();
    }
    for(map<int,pdsp::PatchNode>::iterator it = this->pdspOut.begin(); it != this->pdspOut.end(); it++ ){
        it->second.disconnectAll();
    }
}

//--------------------------------------------------------------
void pdspADSR::loadAudioSettings(){
    ofxXmlSettings XML;

    if (XML.loadFile(patchFile)){
        if (XML.pushTag("settings")){
            sampleRate = XML.getValue("sample_rate_in",0);
            bufferSize = XML.getValue("buffer_size",0);

            /*for(int i=0;i<bufferSize;i++){
                static_cast<vector<float> *>(_outletParams[1])->push_back(0.0f);
            }*/

            XML.popTag();
        }
    }
}

//--------------------------------------------------------------
void pdspADSR::audioOutObject(ofSoundBuffer &outputBuffer){
    // SIGNAL BUFFER
    static_cast<ofSoundBuffer *>(_outletParams[0])->copyFrom(scope.getBuffer().data(), bufferSize, 1, sampleRate);
}

//--------------------------------------------------------------
void pdspADSR::mouseMovedObjectContent(ofVec3f _m){
    gui->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    header->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    duration->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    attackHardness->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    releaseHardness->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));

    for(int j=0;j<static_cast<int>(controlPoints.size());j++){
        controlPoints.at(j).over(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    }

    if(!header->getIsCollapsed()){
        this->isOverGUI = header->hitTest(_m-this->getPos()) || duration->hitTest(_m-this->getPos()) || attackHardness->hitTest(_m-this->getPos()) || releaseHardness->hitTest(_m-this->getPos()) || controlPoints.at(0).bOver || controlPoints.at(1).bOver || controlPoints.at(2).bOver;
    }else{
        this->isOverGUI = header->hitTest(_m-this->getPos()) || controlPoints.at(0).bOver || controlPoints.at(1).bOver || controlPoints.at(2).bOver;
    }

}

//--------------------------------------------------------------
void pdspADSR::dragGUIObject(ofVec3f _m){
    if(this->isOverGUI){
        gui->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        header->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        duration->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        attackHardness->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        releaseHardness->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));

        for(int j=0;j<static_cast<int>(controlPoints.size());j++){
            if(static_cast<int>(_m.x - this->getPos().x) >= rect.getLeft() && static_cast<int>(_m.x - this->getPos().x) <= rect.getRight()){
                controlPoints.at(j).drag(static_cast<int>(_m.x - this->getPos().x),controlPoints.at(j).y);
            }

        }
        this->setCustomVar(controlPoints.at(0).x,"ATTACK_X");
        this->setCustomVar(controlPoints.at(0).y,"ATTACK_Y");
        this->setCustomVar(controlPoints.at(1).x,"DECAY_X");
        this->setCustomVar(controlPoints.at(1).y,"DECAY_Y");
        this->setCustomVar(controlPoints.at(2).x,"SUSTAIN_X");
        this->setCustomVar(controlPoints.at(2).y,"SUSTAIN_Y");
        this->setCustomVar(controlPoints.at(3).x,"RELEASE_X");
        this->setCustomVar(controlPoints.at(3).y,"RELEASE_Y");

    }else{
        ofNotifyEvent(dragEvent, nId);

        box->setFromCenter(_m.x, _m.y,box->getWidth(),box->getHeight());
        headerBox->set(box->getPosition().x,box->getPosition().y,box->getWidth(),headerHeight);

        x = box->getPosition().x;
        y = box->getPosition().y;

        for(int j=0;j<static_cast<int>(outPut.size());j++){
            outPut[j]->linkVertices[0].move(outPut[j]->posFrom.x,outPut[j]->posFrom.y);
            outPut[j]->linkVertices[1].move(outPut[j]->posFrom.x+20,outPut[j]->posFrom.y);
        }
    }
}

//--------------------------------------------------------------
void pdspADSR::onTextInputEvent(ofxDatGuiTextInputEvent e){
    if(!header->getIsCollapsed()){
        if(e.target == duration){
            if(isInteger(e.text)){
                envelopeDuration = ofToInt(e.text);
                this->setCustomVar(ofToFloat(e.text),"DURATION");
            }else{
                duration->setText(ofToString(envelopeDuration));
            }
        }
    }
}

//--------------------------------------------------------------
void pdspADSR::onSliderEvent(ofxDatGuiSliderEvent e){
    if(!header->getIsCollapsed()){
        if(e.target == attackHardness){
            env.setAttackCurve(static_cast<float>(e.value));
            this->setCustomVar(static_cast<float>(e.value),"ATTACK_CURVE");
        }else if(e.target == releaseHardness){
            env.setReleaseCurve(static_cast<float>(e.value));
            this->setCustomVar(static_cast<float>(e.value),"RELEASE_CURVE");
        }
    }
}
