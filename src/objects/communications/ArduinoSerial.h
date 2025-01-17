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

#pragma once

#include "PatchObject.h"

#define MAX_ARDUINO_SENDING_VECTOR_LENGTH 24
#define MAX_ARDUINO_RECEIVING_VECTOR_LENGTH 64

class ArduinoSerial : public PatchObject {

public:

    ArduinoSerial();

    void            newObject();
    void            setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow);
    void            updateObjectContent(map<int,PatchObject*> &patchObjects, ofxThreadedFileDialog &fd);
    void            drawObjectContent(ofxFontStash *font);
    void            removeObjectContent();

    void            mouseMovedObjectContent(ofVec3f _m);
    void            dragGUIObject(ofVec3f _m);

    void            resetSERIALSettings(int devID,int br);

    void            onMatrixEvent(ofxDatGuiMatrixEvent e);
    void            onDropdownEvent(ofxDatGuiDropdownEvent e);

    ofSerial                    serial;
    vector <ofSerialDeviceInfo> deviceList;
    vector<string>              deviceNameList;
    vector<string>              baudrateList;
    int                         serialDeviceID;
    int                         baudRateID;

    size_t                      resetTime;
    string                      temp;

    unsigned char               bytesReturned[MAX_ARDUINO_RECEIVING_VECTOR_LENGTH];
    char                        bytesReadString[MAX_ARDUINO_RECEIVING_VECTOR_LENGTH+1];

    ofImage                     *arduinoIcon;

    ofxDatGui*                  gui;
    ofxDatGuiHeader*            header;
    ofxDatGuiLabel*             serialDeviceName;
    ofxDatGuiMatrix*            deviceSelector;
    ofxDatGuiDropdown*          baudRates;

};
