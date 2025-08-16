#include "IOSCameraCaptureBridge.h"
#include "ISLog.h"

#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>
#import <Accelerate/Accelerate.h>

static CVPixelBufferRef CopyScaledBGRA256(CVPixelBufferRef src)
{
    const size_t dstW = 256, dstH = 256;
    if (CVPixelBufferGetPixelFormatType(src) != kCVPixelFormatType_32BGRA) return NULL;

    CVPixelBufferLockBaseAddress(src, kCVPixelBufferLock_ReadOnly);
    void* srcBase = CVPixelBufferGetBaseAddress(src);
    size_t srcW = CVPixelBufferGetWidth(src);
    size_t srcH = CVPixelBufferGetHeight(src);
    size_t srcStride = CVPixelBufferGetBytesPerRow(src);

    CVPixelBufferRef dst = NULL;
    NSDictionary* attrs = @{ (id)kCVPixelBufferCGImageCompatibilityKey:@YES, (id)kCVPixelBufferCGBitmapContextCompatibilityKey:@YES };
    if (CVPixelBufferCreate(kCFAllocatorDefault, dstW, dstH, kCVPixelFormatType_32BGRA, (CFDictionaryRef)attrs, &dst) != kCVReturnSuccess) {
        CVPixelBufferUnlockBaseAddress(src, kCVPixelBufferLock_ReadOnly); return NULL;
    }

    CVPixelBufferLockBaseAddress(dst, 0);
    void* dstBase = CVPixelBufferGetBaseAddress(dst);
    size_t dstStride = CVPixelBufferGetBytesPerRow(dst);

    vImage_Buffer vSrc = { srcBase, srcH, srcW, srcStride };
    vImage_Buffer vDst = { dstBase, dstH, dstW, dstStride };
    vImageScale_ARGB8888(&vSrc, &vDst, NULL, kvImageHighQualityResampling);

    CVPixelBufferUnlockBaseAddress(dst, 0);
    CVPixelBufferUnlockBaseAddress(src, kCVPixelBufferLock_ReadOnly);
    return dst;
}

@interface ISCameraCapture : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    void* _user;
    void (*_handler)(void*, CVPixelBufferRef);
    AVCaptureSession* _session;
    AVCaptureVideoDataOutput* _output;
    dispatch_queue_t _queue;
}
- (void)registerUser:(void*)user handler:(void(*)(void*, CVPixelBufferRef))handler;
- (BOOL)start; - (void)stop;
@end

@implementation ISCameraCapture
- (instancetype)init { if ((self=[super init])) { _session=[AVCaptureSession new]; _output=[AVCaptureVideoDataOutput new]; _queue=dispatch_queue_create("intellispace.camera", DISPATCH_QUEUE_SERIAL);} return self; }
- (void)dealloc { [self stop]; [_session release]; [_output release]; dispatch_release(_queue); [super dealloc]; }
- (void)registerUser:(void*)user handler:(void(*)(void*, CVPixelBufferRef))handler { _user=user; _handler=handler; }
- (BOOL)start {
    AVCaptureDevice* dev=[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo]; if(!dev) return NO;
    NSError* err=nil; AVCaptureDeviceInput* in=[AVCaptureDeviceInput deviceInputWithDevice:dev error:&err]; if(err||!in) return NO;
    [_session beginConfiguration]; if([_session canAddInput:in]) [_session addInput:in];
    _output.alwaysDiscardsLateVideoFrames=YES; _output.videoSettings=@{ (id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA) };
    [_output setSampleBufferDelegate:self queue:_queue]; if([_session canAddOutput:_output]) [_session addOutput:_output];
    if([_session canSetSessionPreset:AVCaptureSessionPreset1280x720]) _session.sessionPreset=AVCaptureSessionPreset1280x720;
    [_session commitConfiguration]; [_session startRunning];
    UE_LOG(LogIntelliSpace, Log, TEXT("[Camera/iOS] Session started."));
    return YES;
}
- (void)stop { if(_session.isRunning) { [_session stopRunning]; UE_LOG(LogIntelliSpace, Log, TEXT("[Camera/iOS] Session stopped.")); } }
- (void)captureOutput:(AVCaptureOutput*)o didOutputSampleBuffer:(CMSampleBufferRef)sb fromConnection:(AVCaptureConnection*)c {
    if(!_handler) return; CVPixelBufferRef px=CMSampleBufferGetImageBuffer(sb); if(!px) return;
    CVPixelBufferRef scaled=CopyScaledBGRA256(px); if(!scaled) return;
    UE_LOG(LogIntelliSpace, VeryVerbose, TEXT("[Camera/iOS] Frame dispatched 256x256 BGRA."));
    _handler(_user, scaled); CVPixelBufferRelease(scaled);
}
@end

static ISCameraCapture* GCap=nil;

extern "C" {
void RegisterCameraHandler(void* User, void (*Handler)(void*, CVPixelBufferRef)){ if(!GCap) GCap=[ISCameraCapture new]; [GCap registerUser:User handler:Handler]; }
bool StartCameraCapture(){ if(!GCap) GCap=[ISCameraCapture new]; return [GCap start]; }
void StopCameraCapture(){ if(GCap){ [GCap stop]; [GCap release]; GCap=nil; } }
}
#endif
