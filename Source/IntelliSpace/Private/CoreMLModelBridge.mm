#ifdef check
#undef check
#endif
#ifdef NO
#undef NO
#endif
#import <opencv2/opencv.hpp>
#define check(expr) UE_CHECK_IMPL(expr)
#import "CoreMLModelBridge.h"
#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#if PLATFORM_IOS
#import <UIKit/UIKit.h>
#endif
#import <AVFoundation/AVFoundation.h>
#import "FurniLife.h"

#if PLATFORM_IOS
// ---------------------------
// 🔁 Global references
// ---------------------------
@interface CameraCaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@end

@implementation CameraCaptureDelegate

- (void)captureOutput:(AVCaptureOutput*)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection*)connection
{
    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!pixelBuffer) return;

    CFRetain(pixelBuffer); // Retain it until processing is done

//    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
////        std::vector<float> output;
////        FCoreMLModelBridge* bridge = GetSharedBridge(); // Create or hold global bridge instance
////        bridge->RunModel(pixelBuffer, output);
//
//        // Now call back to Unreal
//        AsyncTask(ENamedThreads::GameThread, [pixelBuffer]() {
//            if (AFurniLife::CurrentInstance)
//                AFurniLife::CurrentInstance->OnCameraFrameFromPixelBuffer(pixelBuffer);
//
//            CFRelease(pixelBuffer); // Done
//        });
//    });
}
@end
CameraCaptureDelegate* gDelegate = nil;
AVCaptureSession* gSession = nil;

// ---------------------------
// 🔁 Start Native Camera
// ---------------------------
void StartNativeCamera()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        gSession = [[AVCaptureSession alloc] init];
        gSession.sessionPreset = AVCaptureSessionPreset640x480;

        AVCaptureDevice* device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        NSError* error = nil;
        AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
        if (error || ![gSession canAddInput:input]) {
            NSLog(@"❌ Failed to add camera input: %@", error.localizedDescription);
            return;
        }
        [gSession addInput:input];

        AVCaptureVideoDataOutput* output = [[AVCaptureVideoDataOutput alloc] init];
        output.videoSettings = @{ (__bridge NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) };
        output.alwaysDiscardsLateVideoFrames = YES;

        gDelegate = [[CameraCaptureDelegate alloc] init];
        [output setSampleBufferDelegate:gDelegate queue:dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0)];

        if ([gSession canAddOutput:output]) {
            [gSession addOutput:output];
        } else {
            NSLog(@"❌ Cannot add AVCaptureVideoDataOutput");
            return;
        }

        [gSession startRunning];
        NSLog(@"📸 AVCaptureSession started");
    });
}



UIImage* UIImageFromCVMat(const cv::Mat& mat)
{
    CV_Assert(mat.isContinuous());

    int width = mat.cols;
    int height = mat.rows;
    int channels = mat.channels();

    CGColorSpaceRef colorSpace = nullptr;
    CGBitmapInfo bitmapInfo = 0;

    if (channels == 1)
    {
        colorSpace = CGColorSpaceCreateDeviceGray();
        bitmapInfo = kCGImageAlphaNone;
    }
    else if (channels == 3)
    {
        colorSpace = CGColorSpaceCreateDeviceRGB();
        bitmapInfo = kCGImageByteOrderDefault;
    }
    else if (channels == 4)
    {
        colorSpace = CGColorSpaceCreateDeviceRGB();
        bitmapInfo = kCGImageByteOrderDefault | kCGImageAlphaLast;
    }
    else
    {
        NSLog(@"Unsupported number of channels: %d", channels);
        return nil;
    }

    CGContextRef context = CGBitmapContextCreate(
        (void*)mat.data,
        width,
        height,
        8,
        mat.step[0],
        colorSpace,
        bitmapInfo
    );

    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    UIImage* image = [UIImage imageWithCGImage:cgImage];

    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);

    return image;
}

// ---------------------------
// 👤 Input Wrapper Class
// ---------------------------
@interface U2NetInput : NSObject<MLFeatureProvider>
{
    CVPixelBufferRef _image;
}

@end

@implementation U2NetInput

- (void)setImage:(CVPixelBufferRef)image {
    if (_image != image) {
        if (_image) {
            CVPixelBufferRelease(_image);
        }
        _image = image;
        if (_image) {
            CVPixelBufferRetain(_image);
        }
    }
}

- (CVPixelBufferRef)image {
    return _image;
}
- (NSSet<NSString *> *)featureNames {
    return [NSSet setWithObject:@"input"];  // Change "input" if your model has different input name
}
- (MLFeatureValue *)featureValueForName:(NSString *)name {
    if ([name isEqualToString:@"input"]) {
        return [MLFeatureValue featureValueWithPixelBuffer:_image];
    }
    return nil;
}
@end

// ---------------------------
// 🧠 Model Inference Runner
// ---------------------------
@interface U2NetRunner : NSObject
@property (nonatomic, strong) MLModel* model;
- (BOOL)loadModelAt:(NSString*)path;
- (NSArray<NSNumber*>*)runWithImage:(UIImage*)img;
@end

@implementation U2NetRunner
- (BOOL)loadModelAt:(NSString*)path {
    NSURL* modelURL = [NSURL fileURLWithPath:path];
    NSError* error = nil;
    self.model = [MLModel modelWithContentsOfURL:modelURL error:&error];
    if (error) {
        NSLog(@"❌ Model loading error: %@", error.localizedDescription);
    }
    return (self.model != nil);
}

- (NSArray<NSNumber*>*)runWithImage:(UIImage*)img {
    CVPixelBufferRef buffer = NULL;

    NSDictionary* opts = @{
        (__bridge NSString*)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (__bridge NSString*)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES
    };

    CGSize size = CGSizeMake(320, 320);  // Must match model input
    CVPixelBufferCreate(kCFAllocatorDefault, size.width, size.height,
                        kCVPixelFormatType_32BGRA, (__bridge CFDictionaryRef)opts, &buffer);

    UIGraphicsBeginImageContext(size);
    [img drawInRect:CGRectMake(0, 0, size.width, size.height)];
    UIImage* resized = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();

    CIImage* ci = [[CIImage alloc] initWithImage:resized];
    CIContext* ctx = [CIContext context];
    [ctx render:ci toCVPixelBuffer:buffer];

    U2NetInput* input = [U2NetInput new];
    [input setImage:buffer];
    NSError* err = nil;
    id<MLFeatureProvider> result = [self.model predictionFromFeatures:input error:&err];

    CVPixelBufferRelease(buffer);

    if (!result || err) {
        NSLog(@"❌ Inference error: %@", err.localizedDescription);
        return nil;
    }

    MLFeatureValue* feature = [result featureValueForName:@"output"];
    if (!feature || feature.type != MLFeatureTypeMultiArray) {
        NSLog(@"❌ Output feature 'output' is missing or wrong type");
        return nil;
    }
    MLMultiArray* arr = feature.multiArrayValue;

    NSMutableArray* output = [NSMutableArray arrayWithCapacity:arr.count];
    for (NSUInteger i = 0; i < arr.count; i++)
        [output addObject:arr[i]];

    return output;
}

- (NSArray<NSNumber*>*)runWithPixelBuffer:(CVPixelBufferRef)buffer {
    U2NetInput* input = [U2NetInput new];
    [input setImage:buffer];

    NSError* err = nil;
    id<MLFeatureProvider> result = [self.model predictionFromFeatures:input error:&err];
    if (!result || err) {
        NSLog(@"❌ Inference error (pixel buffer): %@", err.localizedDescription);
        return nil;
    }
    NSLog(@"✅ Available output featuresSSSSSSSSSSSSSS: %@", result.featureNames);


    MLFeatureValue* feature = [result featureValueForName:@"out_p0"];
    if (!feature || feature.type != MLFeatureTypeImage) {
        NSLog(@"❌ Output feature 'out_p6' is missing or not an image.");
        return nil;
    }
    CVPixelBufferRef outBuffer = feature.imageBufferValue;

    CVPixelBufferLockBaseAddress(outBuffer, kCVPixelBufferLock_ReadOnly);

    size_t width = CVPixelBufferGetWidth(outBuffer);
    size_t height = CVPixelBufferGetHeight(outBuffer);
    size_t stride = CVPixelBufferGetBytesPerRow(outBuffer);
    void* base = CVPixelBufferGetBaseAddress(outBuffer);

    // Usually single-channel 8-bit
    uint8_t* pixels = (uint8_t*)base;
    NSMutableArray* output = [NSMutableArray arrayWithCapacity:width * height];

    for (size_t y = 0; y < height; y++) {
        uint8_t* row = pixels + y * stride;
        for (size_t x = 0; x < width; x++) {
            float normalized = row[x] / 255.0f;
            [output addObject:@(normalized)];
        }
    }

    CVPixelBufferUnlockBaseAddress(outBuffer, kCVPixelBufferLock_ReadOnly);
    return output;

}
@end

// ---------------------------
// 🔗 C++ Interface
// ---------------------------
struct FCoreMLImpl {
    U2NetRunner* runner = [[U2NetRunner alloc] init];
};

FCoreMLModelBridge::FCoreMLModelBridge() {
    Impl = new FCoreMLImpl();
}

FCoreMLModelBridge::~FCoreMLModelBridge() {
    delete static_cast<FCoreMLImpl*>(Impl);
}

bool FCoreMLModelBridge::LoadModel(const char* ModelPath) {
    NSString* Path = [NSString stringWithUTF8String:ModelPath];
    FCoreMLImpl* CoreML = static_cast<FCoreMLImpl*>(Impl);
    return [CoreML->runner loadModelAt:Path];

}

bool FCoreMLModelBridge::RunModel(const cv::Mat& InputImage, std::vector<float>& OutputData) {
    UIImage* img = UIImageFromCVMat(InputImage);
    if (!img)
    {
        NSLog(@"❌ Failed to convert cv::Mat to UIImage");
        return false;
    }

    FCoreMLImpl* CoreML = static_cast<FCoreMLImpl*>(Impl);
    NSArray<NSNumber*>* out = [CoreML->runner runWithImage:img];


    if (!out)
        return false;

    OutputData.resize(out.count);
    for (NSUInteger i = 0; i < out.count; ++i)
        OutputData[i] = out[i].floatValue;
    return true;
}

bool FCoreMLModelBridge::RunModel(CVPixelBufferRef PixelBuffer, std::vector<float>& OutputData) {
    FCoreMLImpl* CoreML = static_cast<FCoreMLImpl*>(Impl);
    NSArray<NSNumber*>* out = [CoreML->runner runWithPixelBuffer:PixelBuffer];

    if (!out || out.count == 0)
    {
        AsyncTask(ENamedThreads::GameThread, []() {
            UE_LOG(LogTemp, Error, TEXT("❌ CoreML inference failed or output empty."));
        });
        return false;
    }

    // Convert NSArray to std::vector
    OutputData.resize(out.count);
    for (NSUInteger i = 0; i < out.count; ++i)
        OutputData[i] = out[i].floatValue;

    return true;
}

extern "C" void StartCameraStreaming() {
    StartNativeCamera();
}

FCoreMLModelBridge* GetSharedBridge() {
    static FCoreMLModelBridge* Singleton = new FCoreMLModelBridge();
    return Singleton;
}

#endif
