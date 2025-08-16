#include "U2NetRunnerBridge.h"
#include "ISLog.h"
#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <CoreVideo/CoreVideo.h>

@interface ISU2Net : NSObject
@property(nonatomic,strong) MLModel* model;
@end
@implementation ISU2Net @end

static NSURL* FindURL(NSString* rel){ NSString* base=[[NSBundle mainBundle] bundlePath]; return [NSURL fileURLWithPath:[base stringByAppendingPathComponent:rel] isDirectory:YES]; }

extern "C" {
void* CreateU2NetRunner(){ return (void*)[[ISU2Net alloc] init]; }
bool  LoadU2NetCompiled(void* r){
    NSError* e=nil; NSURL* url=FindURL(@"Content/ML/U2Net.mlmodelc"); if(![[NSFileManager defaultManager] fileExistsAtPath:url.path]) return false;
    MLModelConfiguration* cfg=[MLModelConfiguration new]; cfg.computeUnits=MLComputeUnitsAll;
    ((ISU2Net*)r).model=[MLModel modelWithContentsOfURL:url configuration:cfg error:&e];
    UE_LOG(LogIntelliSpace, Log, TEXT("[U2Net/iOS] LoadCompiled err=%s"), e? *FString([e.localizedDescription UTF8String]) : TEXT("none"));
    return (((ISU2Net*)r).model && !e);
}
bool  LoadU2NetPackage(void* r){
    NSError* e=nil; NSURL* pkg=FindURL(@"Content/ML/U2Net.mlpackage"); if(![[NSFileManager defaultManager] fileExistsAtPath:pkg.path]) return false;
    NSURL* comp=[MLModel compileModelAtURL:pkg error:&e]; if(!comp||e) return false;
    MLModelConfiguration* cfg=[MLModelConfiguration new]; cfg.computeUnits=MLComputeUnitsAll;
    ((ISU2Net*)r).model=[MLModel modelWithContentsOfURL:comp configuration:cfg error:&e];
    UE_LOG(LogIntelliSpace, Log, TEXT("[U2Net/iOS] LoadPackage err=%s"), e? *FString([e.localizedDescription UTF8String]) : TEXT("none"));
    return (((ISU2Net*)r).model && !e);
}
bool  RunU2Net(void* r, CVPixelBufferRef input, float* outMask, int* outW, int* outH){
    ISU2Net* x=(ISU2Net*)r; if(!x.model) return false; NSError* e=nil;
    MLFeatureValue* fin=[MLFeatureValue featureValueWithPixelBuffer:input];
    id<MLFeatureProvider> in=[[MLDictionaryFeatureProvider alloc] initWithDictionary:@{@"input_image":fin} error:&e]; if(e) return false;
    id<MLFeatureProvider> out=[x.model predictionFromFeatures:in error:&e]; if(e||!out) return false;
    NSString* key=[out.featureNames anyObject]; MLFeatureValue* fv=[out featureValueForName:key];
    if(fv.type==MLFeatureTypeMultiArray){ MLMultiArray* arr=fv.multiArrayValue; NSInteger rnk=arr.shape.count;
        int H=(int)[arr.shape[rnk-2] intValue], W=(int)[arr.shape[rnk-1] intValue]; if(outW)*outW=W; if(outH)*outH=H;
        const float* src=(const float*)arr.dataPointer; for(int i=0;i<W*H;++i) outMask[i]=src[i]; return true; }
    return false;
}
void  DestroyU2NetRunner(void* r){ [(__bridge ISU2Net*)r release]; }
}
#endif
