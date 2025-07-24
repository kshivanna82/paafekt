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
#import <UIKit/UIKit.h>


// ---------------------------
// 🔁 cv::Mat to UIImage Helper
// ---------------------------
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

    MLMultiArray* arr = [result featureValueForName:@"output"].multiArrayValue;  // Change name if needed
    NSMutableArray* output = [NSMutableArray arrayWithCapacity:arr.count];
    for (NSUInteger i = 0; i < arr.count; i++)
        [output addObject:arr[i]];

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
