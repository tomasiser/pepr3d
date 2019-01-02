/**
 * Modified for Pepr3D to prevent flipping of camera
 */

/*
 Copyright (c) 2015, The Cinder Project: http://libcinder.org All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Portions of this code (C) Paul Houx
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "ui/CameraUi.h"

namespace pepr3d {

CameraUi::CameraUi()
    : mCamera(nullptr),
      mWindowSize(640, 480),
      mMouseWheelMultiplier(1.2f),
      mMinimumPivotDistance(0.1f),
      mEnabled(true),
      mLastAction(ACTION_NONE) {}

CameraUi::CameraUi(ci::CameraPersp *camera, const ci::app::WindowRef &window, int signalPriority)
    : mCamera(camera), mWindowSize(640, 480), mMouseWheelMultiplier(1.2f), mMinimumPivotDistance(0.1f), mEnabled(true) {
    connect(window, signalPriority);
}

CameraUi::CameraUi(const CameraUi &rhs)
    : mCamera(rhs.mCamera),
      mWindowSize(rhs.mWindowSize),
      mWindow(rhs.mWindow),
      mSignalPriority(rhs.mSignalPriority),
      mMouseWheelMultiplier(rhs.mMouseWheelMultiplier),
      mMinimumPivotDistance(rhs.mMinimumPivotDistance),
      mEnabled(rhs.mEnabled) {
    connect(mWindow, mSignalPriority);
}

CameraUi::~CameraUi() {
    disconnect();
}

CameraUi &CameraUi::operator=(const CameraUi &rhs) {
    mCamera = rhs.mCamera;
    mWindowSize = rhs.mWindowSize;
    mMouseWheelMultiplier = rhs.mMouseWheelMultiplier;
    mMinimumPivotDistance = rhs.mMinimumPivotDistance;
    mWindow = rhs.mWindow;
    mSignalPriority = rhs.mSignalPriority;
    mEnabled = rhs.mEnabled;
    connect(mWindow, mSignalPriority);
    return *this;
}

//! Connects to mouseDown, mouseDrag, mouseWheel and resize signals of \a window, with optional priority \a
//! signalPriority
void CameraUi::connect(const ci::app::WindowRef &window, int signalPriority) {
    if(!mConnections.empty()) {
        disconnect();
    }

    mWindow = window;
    mSignalPriority = signalPriority;
    if(window) {
        mConnections.push_back(window->getSignalMouseDown().connect(
            signalPriority, [this](ci::app::MouseEvent &event) { mouseDown(event); }));
        mConnections.push_back(
            window->getSignalMouseUp().connect(signalPriority, [this](ci::app::MouseEvent &event) { mouseUp(event); }));
        mConnections.push_back(window->getSignalMouseDrag().connect(
            signalPriority, [this](ci::app::MouseEvent &event) { mouseDrag(event); }));
        mConnections.push_back(window->getSignalMouseWheel().connect(
            signalPriority, [this](ci::app::MouseEvent &event) { mouseWheel(event); }));
        mConnections.push_back(window->getSignalResize().connect(signalPriority, [this]() {
            setWindowSize(mWindow->getSize());
            if(mCamera)
                mCamera->setAspectRatio(mWindow->getAspectRatio());
        }));
    }

    mLastAction = ACTION_NONE;
}

//! Disconnects all signal handlers
void CameraUi::disconnect() {
    for(auto &conn : mConnections)
        conn.disconnect();
    mConnections.clear();

    mWindow.reset();
}

bool CameraUi::isConnected() const {
    return mWindow != nullptr;
}

ci::signals::Signal<void()> &CameraUi::getSignalCameraChange() {
    return mSignalCameraChange;
}

void CameraUi::mouseDown(ci::app::MouseEvent &event) {
    if(!mEnabled)
        return;

    mouseDown(event.getPos());
    event.setHandled();
}

void CameraUi::mouseUp(ci::app::MouseEvent &event) {
    if(!mEnabled)
        return;

    mouseUp(event.getPos());
    event.setHandled();
}

void CameraUi::mouseWheel(ci::app::MouseEvent &event) {
    if(!mEnabled)
        return;

    mouseWheel(event.getWheelIncrement());
    event.setHandled();
}

void CameraUi::mouseUp(const glm::vec2 & /*mousePos*/) {
    mLastAction = ACTION_NONE;
}

void CameraUi::mouseDown(const glm::vec2 &mousePos) {
    if(!mCamera || !mEnabled)
        return;

    mInitialMousePos = mousePos;
    mInitialCam = *mCamera;
    mInitialPivotDistance = mCamera->getPivotDistance();
    mLastAction = ACTION_NONE;
}

void CameraUi::mouseDrag(ci::app::MouseEvent &event) {
    if(!mEnabled)
        return;

    bool isLeftDown = event.isLeftDown();
    bool isMiddleDown = event.isMiddleDown() || event.isAltDown();
    bool isRightDown = event.isRightDown() || event.isControlDown();

    if(isMiddleDown)
        isLeftDown = false;

    mouseDrag(event.getPos(), isLeftDown, isMiddleDown, isRightDown);
    event.setHandled();
}

void CameraUi::mouseDrag(const glm::vec2 &mousePos, bool leftDown, bool middleDown, bool rightDown) {
    if(!mCamera || !mEnabled)
        return;

    int action;
    if(rightDown || (leftDown && middleDown))
        action = ACTION_ZOOM;
    else if(middleDown)
        action = ACTION_PAN;
    else if(leftDown)
        action = ACTION_TUMBLE;
    else
        return;

    if(action != mLastAction) {
        mInitialCam = *mCamera;
        mInitialPivotDistance = mCamera->getPivotDistance();
        mInitialMousePos = mousePos;
        mDeltaYBeforeFlip = 0.0f;
    }

    mLastAction = action;

    if(action == ACTION_ZOOM) {  // zooming
        auto mouseDelta = (mousePos.x - mInitialMousePos.x) + (mousePos.y - mInitialMousePos.y);

        float newPivotDistance =
            std::pow(2.71828183f, 2.0f * -mouseDelta / glm::length(glm::vec2(getWindowSize()))) * mInitialPivotDistance;
        glm::vec3 oldTarget = mInitialCam.getEyePoint() + mInitialCam.getViewDirection() * mInitialPivotDistance;
        glm::vec3 newEye = oldTarget - mInitialCam.getViewDirection() * newPivotDistance;
        mCamera->setEyePoint(newEye);
        mCamera->setPivotDistance(std::max<float>(newPivotDistance, mMinimumPivotDistance));
    } else if(action == ACTION_PAN) {  // panning
        float deltaX = (mousePos.x - mInitialMousePos.x) / (float)getWindowSize().x * mInitialPivotDistance;
        float deltaY = (mousePos.y - mInitialMousePos.y) / (float)getWindowSize().y * mInitialPivotDistance;
        glm::vec3 right, up;
        mInitialCam.getBillboardVectors(&right, &up);
        mCamera->setEyePoint(mInitialCam.getEyePoint() - right * deltaX + up * deltaY);
    } else {  // tumbling
        float deltaX = (mousePos.x - mInitialMousePos.x) / -100.0f;
        float deltaY = (mousePos.y - mInitialMousePos.y) / 100.0f;
        glm::vec3 mW = normalize(mInitialCam.getViewDirection());
        bool invertMotion = (mInitialCam.getOrientation() * mInitialCam.getWorldUp()).y < 0.0f;

        glm::vec3 mU = normalize(cross(mInitialCam.getWorldUp(), mW));

        if(invertMotion) {
            deltaX = -deltaX;
            deltaY = -deltaY;
        }

        glm::quat newOrientation = glm::angleAxis(deltaX, mInitialCam.getWorldUp()) * glm::angleAxis(deltaY, mU) *
                                   mInitialCam.getOrientation();
        if((newOrientation * mInitialCam.getWorldUp()).y < 0.0f) {  // prevent camera flip
            mInitialMousePos.y += 100.0f * (deltaY - mDeltaYBeforeFlip);
            deltaY = mDeltaYBeforeFlip;
            newOrientation = glm::angleAxis(deltaX, mInitialCam.getWorldUp()) * glm::angleAxis(deltaY, mU) *
                             mInitialCam.getOrientation();
        }
        mDeltaYBeforeFlip = deltaY;

        glm::vec3 rotatedVec = glm::angleAxis(deltaY, mU) * (-mInitialCam.getViewDirection() * mInitialPivotDistance);
        rotatedVec = glm::angleAxis(deltaX, mInitialCam.getWorldUp()) * rotatedVec;

        glm::vec3 newEyePoint =
            mInitialCam.getEyePoint() + mInitialCam.getViewDirection() * mInitialPivotDistance + rotatedVec;

        mCamera->setEyePoint(newEyePoint);
        mCamera->setOrientation(newOrientation);
    }

    mSignalCameraChange.emit();
}

void CameraUi::mouseWheel(float increment) {
    if(!mCamera || !mEnabled)
        return;

    // some mice issue mouseWheel events during middle-clicks; filter that out
    if(mLastAction != ACTION_NONE)
        return;

    float multiplier;
    if(mMouseWheelMultiplier > 0)
        multiplier = powf(mMouseWheelMultiplier, increment);
    else
        multiplier = powf(-mMouseWheelMultiplier, -increment);

    if(mIsFovZoomEnabled) {
        // FOV zoom (change field of view):
        float fov = mCamera->getFov();
        fov *= multiplier;
        fov = std::max<float>(std::min<float>(120.0f, fov), 1.0f);
        mCamera->setFov(fov);
    } else {
        // dolly (move closer / further from the pivot point):
        multiplier = std::max<float>(multiplier, mMinimumPivotDistance / mCamera->getPivotDistance());
        glm::vec3 newEye =
            mCamera->getEyePoint() + mCamera->getViewDirection() * (mCamera->getPivotDistance() * (1 - multiplier));
        mCamera->setEyePoint(newEye);
        mCamera->setPivotDistance(std::max<float>(mCamera->getPivotDistance() * multiplier, mMinimumPivotDistance));
    }

    mSignalCameraChange.emit();
}

glm::ivec2 CameraUi::getWindowSize() const {
    if(mWindow)
        return mWindow->getSize();
    else
        return mWindowSize;
}

}  // namespace pepr3d
