// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class IntelliSpace : ModuleRules
{
	public IntelliSpace(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            //PublicDefinitions.Add("UE_ENABLE_RTTI=1");
            //PublicDefinitions.Add("UE_ENABLE_EXCEPTIONS=1");
            bUseRTTI = true;
            bEnableExceptions = true;
        }

        string ProjectRoot = Path.Combine(ModuleDirectory, "../..");
        //string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../.."));


        string ThirdPartyPath = Path.Combine(ProjectRoot, "ThirdParty");
        string ONNXRoot = Path.Combine(ThirdPartyPath, "ONNXRuntime");
        string ONNXIncludePath = Path.Combine(ONNXRoot, "include");
    

        //string ONNXLibPath = Path.Combine(ONNXRoot, "include", "lib", "iOS");
        //string ONNXLibPath = Path.Combine(ONNXIncludePath, "lib", "iOS");
        //PublicIncludePaths.Add(Path.Combine(ONNXRoot, "build_ios", "Release", "_deps", "onnx-src"));

    
    
        // ONNX Headers
        //PublicIncludePaths.Add(ONNXIncludePath);
     
        string ONNXLibPath = "";

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //ONNXLibPath = Path.Combine(ONNXRoot, "include", "lib", "Mac");
            PublicIncludePaths.Add(Path.Combine(ONNXRoot, "include"));
            // Add this only if you built protobuf headers for Mac too
            // PublicIncludePaths.Add(Path.Combine(ONNXRoot, "build_mac", "_deps", "onnx-src", "onnx"));

            //string FullPath = Path.Combine(ModuleDirectory, "Private", "IOSNNERunner.cpp");
            //ExcludedPrivateCompilePaths.Add(FullPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            ONNXLibPath = Path.Combine(ONNXRoot, "include", "lib", "iOS");
            PublicIncludePaths.Add(Path.Combine(ONNXRoot, "include"));
            PublicIncludePaths.Add(Path.Combine(ONNXRoot, "build_ios", "Release", "_deps", "onnx-src", "onnx"));
        }

        
        
        // Common OpenCV modules you depend on
        string[] OpenCVModules = new string[]
        {
                "core", "videoio", "imgproc", "highgui", "imgcodecs"
        };

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
                PublicDefinitions.Add("CLANG_WARNINGS_SUPPRESSED");
                PublicDefinitions.Add("SUPPRESS_LIBCXX_WARNING");

                // Proper workaround for clang++ warning suppression:
                bEnableUndefinedIdentifierWarnings = false; // prevents some aggressive warnings

                string OpenCVPath = "/opt/homebrew/opt/opencv";

                PublicIncludePaths.Add(Path.Combine(OpenCVPath, "include", "opencv4"));

                foreach (string module in OpenCVModules)
                {
                    PublicAdditionalLibraries.Add(Path.Combine(OpenCVPath, "lib", $"libopencv_{module}.dylib"));
                }
        
                              
                PublicDependencyModuleNames.AddRange(new string[]
                {
                        "NNE", "NNERuntimeORT", "NNEUtilities"
                });

                PrivateDependencyModuleNames.AddRange(new string[]
                {
                        "NNE", "NNERuntimeORT"
                });
                PublicIncludePaths.Add(Path.Combine(ONNXRoot, "include"));
        }

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            //string ModelPathForUnet = Path.Combine(ModuleDirectory, "/Users/al/Documents/tries/ML/u2net.mlmodelc");
            //RuntimeDependencies.Add(ModelPathForUnet);
            
            
            string OpenCVModulesPath = "/Volumes/ExtremSSD/tries/opencv";
            string OpenCVLibPath = Path.Combine(OpenCVModulesPath, "build_ios");
            //string OpenCVModuleDirectory = "/Volumes/ExtremSSD/projects/IntelliSpace/Source/IntelliSpace";

            PublicDefinitions.Add("USE_DIRECT_ONNX=1");
            PublicDefinitions.Add("ORT_NO_EXCEPTIONS");

            // ONNX Librariesssssssssssssssss
            string[] ORTLibraries = new string[]
            {
                "libonnx_proto.a",
                "libabsl_city.a",
                "libabsl_time_zone.a",
                "libonnxruntime_session.a",
                "libabsl_spinlock_wait.a",
                "libabsl_cord.a",
                "libabsl_log_severity.a",
                "libabsl_bad_optional_access.a",
                "libabsl_hash.a",
                "libabsl_raw_logging_internal.a",
                "libonnxruntime_framework.a",
                "libabsl_cord_internal.a",
                "libabsl_base.a",
                "libabsl_cordz_info.a",
                "libonnxruntime_flatbuffers.a",
                "libonnxruntime_util.a",
                "libabsl_throw_delegate.a",
                "libabsl_debugging_internal.a",
                "libonnxruntime_common.a",
                "libonnxruntime_mlas.a",
                "libonnxruntime_test_utils.a",
                "libabsl_strings.a",
                "libonnxruntime_optimizer.a",
                "libabsl_malloc_internal.a",
                "libprotobuf-lite.a",
                "libabsl_strings_internal.a",
                "libabsl_int128.a",
                "libabsl_raw_hash_set.a",
                "libonnx.a",
                "libabsl_symbolize.a",
                "libabsl_graphcycles_internal.a",
                "libabsl_exponential_biased.a",
                "libonnxruntime_providers.a",
                "libabsl_bad_variant_access.a",
                "libnsync_cpp.a",
                "libabsl_stacktrace.a",
                "libabsl_cordz_handle.a",
                "libonnxruntime_graph.a",
                "libonnxruntime_combined.a",
                "libabsl_time.a",
                "libabsl_synchronization.a",
                "libabsl_hashtablez_sampler.a",
                "libabsl_demangle_internal.a",
                "libre2.a",
                "libabsl_cordz_functions.a",
                "libabsl_low_level_hash.a",
                "libonnxruntime_mocked_allocator.a",
                "libabsl_civil_time.a"
            };
            foreach (string lib in ORTLibraries)
            {
                PublicAdditionalLibraries.Add(Path.Combine(ONNXLibPath, lib));
            }
      
    
            PublicSystemLibraries.AddRange(new string[] { "z", "iconv", "bz2", "c++" });

            // OpenCV Headers
            string[] OpenCVHeaderModules = new string[]
            {
                "core", "imgproc", "video", "highgui", "imgcodecs", "videoio"
            };

            foreach (string module in OpenCVHeaderModules)
            {
                PublicIncludePaths.Add(Path.Combine(OpenCVModulesPath, "modules", module, "include"));
            }
            
            //PublicIncludePaths.Add(Path.Combine(OpenCVModuleDirectory, "Public"));
            //PrivateIncludePaths.Add(Path.Combine(OpenCVModuleDirectory, "Private"));
            //kish
            //PublicIncludePaths.Add(Path.Combine(OpenCVModulesPath, "/include"));
            //PublicIncludePaths.Add(Path.Combine(OpenCVModulesPath, "/build_ios/install_ios/include/opencv4"));
            
            PublicIncludePaths.Add(Path.Combine(OpenCVModulesPath, "build_ios/install_ios/include"));
            PublicIncludePaths.Add(Path.Combine(OpenCVModulesPath, "build_ios/install_ios/include/opencv4"));



            
            PublicIncludePaths.Add(Path.Combine(ONNXRoot, "include"));
            

            PublicIncludePaths.Add(OpenCVLibPath);

            // OpenCV Static Libraries
            string[] OpenCVLibs = new string[]
            {
                "lib/Release/libopencv_world.a",
                "3rdparty/lib/Release/liblibjpeg-turbo.a",
                "3rdparty/lib/Release/libzlib.a",
                "build/simd.build/Release-iphoneos/libsimd.a",
                "lib/Release/libopencv_ts.a",
                "build/jpeg12-static.build/Release-iphoneos/libjpeg12-static.a",
                "build/jpeg16-static.build/Release-iphoneos/libjpeg16-static.a",
                "3rdparty/lib/Release/liblibpng.a",
                "3rdparty/lib/Release/liblibwebp.a",
                "3rdparty/lib/Release/liblibprotobuf.a",
                "3rdparty/lib/Release/libIlmImf.a",
                "3rdparty/lib/Release/libade.a"
            };

            foreach (string libRelPath in OpenCVLibs)
            {
                PublicAdditionalLibraries.Add(Path.Combine(OpenCVLibPath, libRelPath));
            }


            PublicSystemLibraries.AddRange(new string[] { "z", "iconv", "bz2", "c++" });


            PublicFrameworks.AddRange(new string[]
            {
                "Accelerate", "AVFoundation", "CoreMedia", "CoreVideo", "CoreML", "UIKit", "Vision", "Foundation", "ARKit"
            });
      
            //PrivateIncludePaths.Add("IntelliSpace/Private");
            
            // Add your iOS-specific source files
            //PrivateSourceFiles.Add("Private/iOS/CustomIOSAppDelegate.cpp");
                
        }
     

        PublicDependencyModuleNames.AddRange(new string[]
        {
                "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "Media", "MediaAssets", "ImagePlate", "Projects", "CinematicCamera", "ProceduralMeshComponent", "UMG", "Slate", "SlateCore", "Json", "HTTP"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
                "Core", "CoreUObject", "Engine", "Renderer", "RenderCore", "RHI", "RHICore", "Projects", "ToolMenus", "HTTP", "WebBrowser"
        });
            
       if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[] {
                "EditorStyle", "EditorWidgets"
            });
        }
	}
}
