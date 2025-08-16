#include "CoreMinimal.h"
#include "ISLog.h"
#include <float.h>
#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <CoreVideo/CoreVideo.h>

static NSURL* FindModelURL(NSString* relativePath){
    NSString* bundle=[[NSBundle mainBundle] bundlePath];
    NSString* full=[bundle stringByAppendingPathComponent:relativePath];
    return [NSURL fileURLWithPath:full isDirectory:YES];
}

@interface ISMiDaSRunner : NSObject
@property (nonatomic, strong) MLModel *model;
@end
@implementation ISMiDaSRunner @end

extern "C" {
void* CreateMiDaSRunner(){ return (void*)[[ISMiDaSRunner alloc] init]; }
bool  LoadMiDaSCompiled(void* r){
    NSError* err=nil; NSURL* url=FindModelURL(@"Content/ML/MiDaS_swin2_tiny_256.mlmodelc");
    if(![[NSFileManager defaultManager] fileExistsAtPath:url.path]) return false;
    MLModelConfiguration* cfg=[MLModelConfiguration new]; cfg.computeUnits=MLComputeUnitsAll;
    ((ISMiDaSRunner*)r).model=[MLModel modelWithContentsOfURL:url configuration:cfg error:&err];
    UE_LOG(LogIntelliSpace, Log, TEXT("[MiDaS/iOS] LoadCompiled err=%s"), err? *FString([err.localizedDescription UTF8String]) : TEXT("none"));
    return (((ISMiDaSRunner*)r).model && !err);
}
bool  LoadMiDaSPackage(void* r){
    NSError* err=nil; NSURL* pkg=FindModelURL(@"Content/ML/MiDaS_swin2_tiny_256.mlpackage");
    if(![[NSFileManager defaultManager] fileExistsAtPath:pkg.path]) return false;
    NSURL* compiled=[MLModel compileModelAtURL:pkg error:&err]; if(!compiled||err) return false;
    MLModelConfiguration* cfg=[MLModelConfiguration new]; cfg.computeUnits=MLComputeUnitsAll;
    ((ISMiDaSRunner*)r).model=[MLModel modelWithContentsOfURL:compiled configuration:cfg error:&err];
    UE_LOG(LogIntelliSpace, Log, TEXT("[MiDaS/iOS] LoadPackage err=%s"), err? *FString([err.localizedDescription UTF8String]) : TEXT("none"));
    return (((ISMiDaSRunner*)r).model && !err);
}
bool  RunMiDaS(void* r, CVPixelBufferRef input, float* outDepth){
    ISMiDaSRunner* x=(ISMiDaSRunner*)r; if(!x.model) return false;
    NSError* err=nil;
    MLFeatureValue* fin=[MLFeatureValue featureValueWithPixelBuffer:input];
    id<MLFeatureProvider> in=[[MLDictionaryFeatureProvider alloc] initWithDictionary:@{@"input_image":fin} error:&err];
    if(err) return false;
    id<MLFeatureProvider> out=[x.model predictionFromFeatures:in error:&err];
    if(err||!out) return false;
    NSString* key=[out.featureNames anyObject];
    MLFeatureValue* fv=[out featureValueForName:key];
    if(fv.type==MLFeatureTypeMultiArray){
        MLMultiArray* arr=fv.multiArrayValue; NSInteger rank=arr.shape.count;
        int H=(int)[arr.shape[rank-2] intValue], W=(int)[arr.shape[rank-1] intValue];
        const float* src=(const float*)arr.dataPointer; float mn=FLT_MAX, mx=-FLT_MAX;
        int N=W*H; for(int i=0;i<N;++i){ float v=src[i]; if(v<mn) mn=v; if(v>mx) mx=v; }
        float inv=(mx>mn)?1.f/(mx-mn):1.f; for(int i=0;i<N;++i) outDepth[i]=(src[i]-mn)*inv;
        return true;
    }
    return false;
}
void  DestroyMiDaSRunner(void* r){ [(__bridge ISMiDaSRunner*)r release]; }
}
#else
extern "C" { void* CreateMiDaSRunner(){return nullptr;} bool LoadMiDaSCompiled(void*){return false;}
bool LoadMiDaSPackage(void*){return false;} bool RunMiDaS(void*, void*, float*){return false;} void DestroyMiDaSRunner(void*){} }
#endif
