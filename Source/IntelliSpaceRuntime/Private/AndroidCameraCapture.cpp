#include "PlatformCameraCaptureBridge.h"

#if PLATFORM_ANDROID

#include "ISLog.h"
#include <media/NdkImageReader.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraMetadata.h>

extern "C" {

static void* GUser = nullptr;
static void (*GHandler)(void*, CVPixelBufferRef) = nullptr;

static ACameraManager*       gManager  = nullptr;
static ACameraDevice*        gDevice   = nullptr;
static ACaptureSessionOutputContainer* gOutputs = nullptr;
static ACaptureSessionOutput* gSessionOut = nullptr;
static ACameraOutputTarget*  gTarget   = nullptr;
static ACameraCaptureSession* gSession = nullptr;
static AImageReader*         gReader   = nullptr;
static ANativeWindow*        gWindow   = nullptr;
static ACaptureRequest*      gRequest  = nullptr;
static bool gRunning = false;

static void OnImageAvailable(void* /*ctx*/, AImageReader* reader)
{
    AImage* image = nullptr;
    if (AImageReader_acquireNextImage(reader, &image) != AMEDIA_OK || !image) return;

    int32_t w=0,h=0,fmt=0; AImage_getWidth(image, &w); AImage_getHeight(image, &h); AImage_getFormat(image,&fmt);
    if (fmt == AIMAGE_FORMAT_RGBA_8888)
    {
        uint8_t* data = nullptr; int len = 0;
        if (AImage_getPlaneData(image, 0, &data, &len) == AMEDIA_OK && data && len > 0)
        {
            AndroidFrameRGBA* frame = new AndroidFrameRGBA();
            frame->Width  = w; frame->Height = h;
            int rowStride = 0; AImage_getPlaneRowStride(image, 0, &rowStride);
            frame->Stride = rowStride;
            frame->Size   = (size_t)len;
            frame->Data   = (uint8_t*)malloc(len);
            memcpy(frame->Data, data, len);
            if (GHandler) GHandler(GUser, frame);
            free(frame->Data); frame->Data=nullptr; delete frame;
        }
    }
    AImage_delete(image);
}

void RegisterCameraHandler(void* User, void (*Handler)(void* User, CVPixelBufferRef Pixel))
{
    GUser = User; GHandler = Handler;
    UE_LOG(LogIntelliSpace, Log, TEXT("[Camera/Android] Handler registered."));
}

static const char* PickBackCameraId(ACameraManager* mgr)
{
    ACameraIdList* ids = nullptr;
    if (ACameraManager_getCameraIdList(mgr, &ids) != ACAMERA_OK || !ids || ids->numCameras == 0) return nullptr;
    const char* chosen = ids->cameraIds[0];
    for (int i=0;i<ids->numCameras;++i){
        const char* id = ids->cameraIds[i];
        ACameraMetadata* meta = nullptr;
        if (ACameraManager_getCameraCharacteristics(mgr, id, &meta) == ACAMERA_OK && meta){
            ACameraMetadata_const_entry lensFacing;
            if (ACameraMetadata_getConstEntry(meta, ACAMERA_LENS_FACING, &lensFacing) == ACAMERA_OK){
                if (lensFacing.count>0 && lensFacing.data.u8[0] == ACAMERA_LENS_FACING_BACK){
                    chosen = id;
                }
            }
            ACameraMetadata_free(meta);
        }
    }
    return chosen;
}

bool StartCameraCapture()
{
    if (gRunning) return true;

    gManager = ACameraManager_create();
    if (!gManager) { UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] Manager create failed.")); return false; }

    const char* camId = PickBackCameraId(gManager);
    if (!camId) { UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] No camera id.")); return false; }

    ACameraDevice_stateCallbacks devCbs = {};
    camera_status_t st = ACameraManager_openCamera(gManager, camId, &devCbs, &gDevice);
    if (st != ACAMERA_OK || !gDevice) { UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] openCamera failed %d"), (int)st); return false; }

    if (AImageReader_new(256, 256, AIMAGE_FORMAT_RGBA_8888, 3, &gReader) != AMEDIA_OK || !gReader) {
        UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] ImageReader create failed.")); return false;
    }
    AImageReader_ImageListener lis; lis.context=nullptr; lis.onImageAvailable=&OnImageAvailable;
    AImageReader_setImageListener(gReader, &lis);
    AImageReader_getWindow(gReader, &gWindow);

    ACaptureSessionOutput_create(gWindow, &gSessionOut);
    ACaptureSessionOutputContainer_create(&gOutputs);
    ACaptureSessionOutputContainer_add(gOutputs, gSessionOut);
    ACameraOutputTarget_create(gWindow, &gTarget);

    ACameraDevice_createCaptureRequest(gDevice, TEMPLATE_PREVIEW, &gRequest);
    ACaptureRequest_addTarget(gRequest, gTarget);

    ACameraCaptureSession_stateCallbacks sessCbs = {};
    camera_status_t sessSt = ACameraDevice_createCaptureSession(gDevice, gOutputs, &sessCbs, &gSession);
    if (sessSt != ACAMERA_OK || !gSession) { UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] createCaptureSession failed %d"), (int)sessSt); return false; }

    int sequenceId = 0;
    camera_status_t rep = ACameraCaptureSession_setRepeatingRequest(gSession, nullptr, 1, &gRequest, &sequenceId);
    if (rep != ACAMERA_OK) { UE_LOG(LogIntelliSpace, Error, TEXT("[Camera/Android] setRepeatingRequest failed %d"), (int)rep); return false; }

    gRunning = true;
    UE_LOG(LogIntelliSpace, Log, TEXT("[Camera/Android] Started (RGBA 256x256)."));
    return true;
}

void StopCameraCapture()
{
    if (!gRunning) return;

    if (gSession) { ACameraCaptureSession_stopRepeating(gSession); ACameraCaptureSession_close(gSession); gSession=nullptr; }
    if (gRequest){ ACaptureRequest_removeTarget(gRequest, gTarget); ACaptureRequest_free(gRequest); gRequest=nullptr; }
    if (gTarget) { ACameraOutputTarget_free(gTarget); gTarget=nullptr; }
    if (gOutputs){ if (gSessionOut) { ACaptureSessionOutputContainer_remove(gOutputs, gSessionOut); } ACaptureSessionOutputContainer_free(gOutputs); gOutputs=nullptr; }
    if (gSessionOut){ ACaptureSessionOutput_free(gSessionOut); gSessionOut=nullptr; }
    if (gWindow) { ANativeWindow_release(gWindow); gWindow=nullptr; }
    if (gReader) { AImageReader_delete(gReader); gReader=nullptr; }
    if (gDevice) { ACameraDevice_close(gDevice); gDevice=nullptr; }
    if (gManager){ ACameraManager_delete(gManager); gManager=nullptr; }

    gRunning = false;
    UE_LOG(LogIntelliSpace, Log, TEXT("[Camera/Android] Stopped."));
}

} // extern "C"
#endif // PLATFORM_ANDROID
