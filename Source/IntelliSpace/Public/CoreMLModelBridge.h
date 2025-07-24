#pragma once

#include <opencv2/core.hpp>
#include <vector>

#ifdef __OBJC__
@class U2NetRunner;  // Forward declare the Objective-C class when compiling as Objective-C++
#else
class U2NetRunner;
#endif

class FCoreMLModelBridge
{
public:
    FCoreMLModelBridge();
    ~FCoreMLModelBridge();

    bool LoadModel(const char* ModelPath);
    bool RunModel(const cv::Mat& InputImage, std::vector<float>& OutputData);

private:
    void* Impl;  // Internal ObjC++ object
};
