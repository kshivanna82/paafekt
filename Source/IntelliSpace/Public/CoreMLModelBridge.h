#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <CoreVideo/CoreVideo.h>

#ifdef __OBJC__
@class U2NetRunner;  // Forward declare the Objective-C class when compiling as Objective-C++
#else
class U2NetRunner;
#endif

#ifdef __cplusplus
    extern "C" {
#endif

        void StartCameraStreaming();
        class FCoreMLModelBridge* GetSharedBridge();
    

#ifdef __cplusplus
}
#endif

class FCoreMLModelBridge
{
public:
    FCoreMLModelBridge();
    ~FCoreMLModelBridge();


    bool LoadModel(const char* ModelPath);
    bool RunModel(const cv::Mat& InputImage, std::vector<float>& OutputData);
    bool RunModel(CVPixelBufferRef PixelBuffer, std::vector<float>& OutputData);

private:
    void* Impl;  // Internal ObjC++ object
};
