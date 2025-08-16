// IOSCameraCapture.mm
#import <AVFoundation/AVFoundation.h>
#include "PlatformCameraCaptureBridge.h"

#if PLATFORM_IOS

static FISCameraFrameCallback GCallback = nullptr;
static void* GUserData = nullptr;

extern "C" void RegisterCameraHandler(void* UserData, FISCameraFrameCallback Callback)
{
    GUserData = UserData;
    GCallback = Callback;
}

static dispatch_queue_t CaptureQueue()
{
    static dispatch_queue_t Q;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        Q = dispatch_queue_create("com.intellispace.capture", DISPATCH_QUEUE_SERIAL);
    });
    return Q;
}

@interface FISCamDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@end

@implementation FISCamDelegate
- (void)captureOutput:(AVCaptureOutput*)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection*)connection
{
    if (!GCallback) return;
    CVPixelBufferRef pix = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!pix) return;
    GCallback(GUserData, pix);
}
@end

static AVCaptureSession* GSession = nil;
static FISCamDelegate*   GDelegate = nil;

extern "C" void StartCameraCapture()
{
    if (GSession) return;
    GSession = [AVCaptureSession new];
    GSession.sessionPreset = AVCaptureSessionPreset1280x720;

    AVCaptureDevice* cam = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSError* err = nil;
    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:cam error:&err];
    if (!input) { UE_LOG(LogTemp, Error, TEXT("Camera input error: %s"), *FString(err.localizedDescription)); return; }
    if ([GSession canAddInput:input]) [GSession addInput:input];

    AVCaptureVideoDataOutput* out = [AVCaptureVideoDataOutput new];
    out.alwaysDiscardsLateVideoFrames = YES;
    out.videoSettings = @{ (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) };
    if ([GSession canAddOutput:out]) [GSession addOutput:out];

    GDelegate = [FISCamDelegate new];
    [out setSampleBufferDelegate:GDelegate queue:CaptureQueue()];

    [GSession startRunning];
}

extern "C" void StopCameraCapture()
{
    if (!GSession) return;
    [GSession stopRunning];
    GSession = nil;
    GDelegate = nil;
}

#endif // PLATFORM_IOS
