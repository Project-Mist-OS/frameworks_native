/*
 ** Copyright 2013, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "GLConsumer"

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/String8.h>

#include <private/gui/SyncFeatures.h>

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(SyncFeatures);

SyncFeatures::SyncFeatures() : Singleton<SyncFeatures>(),
        mHasNativeFenceSync(false),
        mHasFenceSync(false),
        mHasWaitSync(false) {
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    // eglQueryString can only be called after EGL has been initialized;
    // otherwise the check below will abort.  If RenderEngine is using SkiaVk,
    // EGL will not have been initialized.  There's no problem with initializing
    // it again here (it is ref counted), and then terminating it later.
    EGLBoolean initialized = eglInitialize(dpy, nullptr, nullptr);
    LOG_ALWAYS_FATAL_IF(!initialized, "eglInitialize failed");
    const char* exts = eglQueryString(dpy, EGL_EXTENSIONS);
    LOG_ALWAYS_FATAL_IF(exts == nullptr, "eglQueryString failed");
    if (strstr(exts, "EGL_ANDROID_native_fence_sync")) {
        // This makes GLConsumer use the EGL_ANDROID_native_fence_sync
        // extension to create Android native fences to signal when all
        // GLES reads for a given buffer have completed.
        mHasNativeFenceSync = true;
    }
    if (strstr(exts, "EGL_KHR_fence_sync")) {
        mHasFenceSync = true;
    }
    if (strstr(exts, "EGL_KHR_wait_sync")) {
        mHasWaitSync = true;
    }
    mString.append("[using:");
    if (useNativeFenceSync()) {
        mString.append(" EGL_ANDROID_native_fence_sync");
    }
    if (useFenceSync()) {
        mString.append(" EGL_KHR_fence_sync");
    }
    if (useWaitSync()) {
        mString.append(" EGL_KHR_wait_sync");
    }
    mString.append("]");
    // Terminate EGL to match the eglInitialize above
    eglTerminate(dpy);
}

bool SyncFeatures::useNativeFenceSync() const {
    // EGL_ANDROID_native_fence_sync is not compatible with using the
    // EGL_KHR_fence_sync extension for the same purpose.
    return mHasNativeFenceSync;
}
bool SyncFeatures::useFenceSync() const {
    return !mHasNativeFenceSync && mHasFenceSync;
}
bool SyncFeatures::useWaitSync() const {
    return (useNativeFenceSync() || useFenceSync()) && mHasWaitSync;
}

String8 SyncFeatures::toString() const {
    return mString;
}

} // namespace android
