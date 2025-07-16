// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class IntelliSpace : ModuleRules
{
	public IntelliSpace(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        	string ProjectRoot = Path.Combine(ModuleDirectory, "../..");
         //string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../.."));


        	string ThirdPartyPath = Path.Combine(ProjectRoot, "ThirdParty");
        	string ONNXRoot = Path.Combine(ThirdPartyPath, "ONNXRuntime");
        	string ONNXIncludePath = Path.Combine(ONNXRoot, "include");
        

        	//string ONNXLibPath = Path.Combine(ONNXRoot, "include", "lib", "iOS");
        	string ONNXLibPath = Path.Combine(ONNXIncludePath, "lib", "iOS");
        	PublicIncludePaths.Add(Path.Combine(ONNXRoot, "build_ios", "Release", "_deps", "onnx-src"));

        
        
        	// ONNX Headers
        	PublicIncludePaths.Add(ONNXIncludePath);
            
            
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
        	}

        	if (Target.Platform == UnrealTargetPlatform.IOS)
        	{
            		string OpenCVModulesPath = "/Volumes/ExtremSSD/tries/opencv";
            		string OpenCVLibPath = Path.Combine(OpenCVModulesPath, "build_ios");

            		PublicDefinitions.Add("ORT_NO_EXCEPTIONS");
            		PublicDefinitions.Add("USE_DIRECT_ONNX=1");

            		// ONNX Headers
            		//PublicIncludePaths.Add(ONNXIncludePath);
            		//PublicIncludePaths.Add(Path.Combine(ONNXIncludePath, "onnxruntime", "core", "session"));

            		// ONNX Libraries
            		string[] ORTLibraries = new string[]
            		{
                		"libonnxruntime_common.a",
                		"libonnxruntime_framework.a",
                		"libonnxruntime_graph.a",
                		"libonnxruntime_session.a",
                		"libonnxruntime_optimizer.a",
                		"libonnxruntime_flatbuffers.a",
                		"libonnxruntime_util.a",
                		"libonnxruntime_providers.a",
                		"libonnxruntime_test_utils.a",
                		"libonnxruntime_mocked_allocator.a",
                		"libonnxruntime_mlas.a",
                        "libonnx.a",
                        "libonnx_proto.a",
                        "libprotobuf-lite.a",
                        "libnsync_cpp.a",
                        "libre2.a",
                        "libabsl_strings.a",
                        "libabsl_base.a",
                        "libabsl_hash.a",
                        "libabsl_raw_hash_set.a",
                        "libabsl_throw_delegate.a",
                        "libabsl_raw_hash_set.a",
                        "libabsl_base.a",
                        "libabsl_strings.a",
                        "libabsl_throw_delegate.a",
                        "libabsl_low_level_hash.a"
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
                		"Accelerate", "AVFoundation", "CoreMedia", "CoreVideo"
            		});
        	}

        	PublicDependencyModuleNames.AddRange(new string[]
        	{
            		"Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "Media", "MediaAssets", "NNE", "ImagePlate"
        	});

        	PrivateDependencyModuleNames.AddRange(new string[]
        	{
            		"Core", "CoreUObject", "Engine", "Renderer", "RenderCore", "RHI", "RHICore", "NNE"
        	});

	}
}
