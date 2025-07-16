// Fill out your copyright notice in the Description page of Project Settings.


#include "FurniLife.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

// Sets default values
AFurniLife::AFurniLife()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFurniLife::BeginPlay()
{
	Super::BeginPlay();
//    FString OnnxPath = FPaths::ProjectContentDir() / TEXT("Game/Models/U2NET1.onnx");
//    FString testFilePath = FPaths::ProjectContentDir() / TEXT("Game/Models/test.txt");
    FString bundlePath = FString([[NSBundle mainBundle] resourcePath]) ;
    UE_LOG(LogTemp, Log, TEXT("bundlePathhhhhhhhhhhhhhhhhhhhhhhhhh exists: %s"), *bundlePath);
    FString OnnxPath = FString([[NSBundle mainBundle] resourcePath]) + TEXT("/Game/Models/U2NET1.onnx");
    FString testFilePath = FString([[NSBundle mainBundle] resourcePath]) + TEXT("/Game/Models/test.txt");
    
    
    FString ExecPath = FPlatformProcess::ExecutablePath();
    UE_LOG(LogTemp, Warning, TEXT("Executable11111111111111 Path: %s"), *ExecPath);

    UE_LOG(LogTemp, Warning, TEXT("Project222222222222222 Dir: %s"), *FPaths::ProjectDir());
    UE_LOG(LogTemp, Warning, TEXT("Content333333333333333 Dir: %s"), *FPaths::ProjectContentDir());
    UE_LOG(LogTemp, Warning, TEXT("Persistent4444444444444444 Dir: %s"), *FPaths::ProjectPersistentDownloadDir());
    
    
    
    FString FolderPath = FPaths::ProjectContentDir();
//    FString FolderPath = FPaths::ProjectContentDir() / TEXT("/Models/");
       
   IFileManager& FileManager = IFileManager::Get();
   TArray<FString> FoundFiles;
   
   FileManager.FindFiles(FoundFiles, *FolderPath, TEXT("*.*")); // List all files

   for (const FString& File : FoundFiles)
   {
       UE_LOG(LogTemp, Log, TEXT("Foundvvvvvvvvvvvvvvvv file in Models/: %s"), *File);
   }



    TArray<uint8> RawData;
//    if (!FFileHelper::LoadFileToArray(RawData, *OnnxPath))
//    {
//        UE_LOG(LogTemp, Error, TEXT("Failed to load ONNX model: %s"), *OnnxPath);
//        return;
//    } else {
//        UE_LOG(LogTemp, Log, TEXT("Loaded ONNX model: %s"), *OnnxPath);
//        UE_LOG(LogTemp, Log, TEXT("Loaded Test File Path: %s"), *testFilePath);
//        return;
//    }
    
//    FString FullPath = FPaths::ProjectContentDir() + TEXT("Models/U2NET1.onnx");


    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*OnnxPath))
    {
        UE_LOG(LogTemp, Log, TEXT("Fileoooooooooooo exists: %s"), *OnnxPath);
        
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Fileoooooooooooo does NOT exist: %s"), *OnnxPath);
    }
    
    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*testFilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Fileeeeeeee exists: %s"), *testFilePath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Fileeeeeeeeee does NOT exist: %s"), *testFilePath);
    }
    
    //#if PLATFORM_IOS
    //    FString ModelPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("/U2NET1.onnx"));
    //    OrtSession = new Ort::Session(OrtEnv, TCHAR_TO_UTF8(*ModelPath), SessionOptions);
//#endif
	
}

// Called every frame
void AFurniLife::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FString AFurniLife::LoadFileToString(FString Filename) {
    FString directory = FPaths::ProjectContentDir();
    FString result;
    IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
    if(file.CreateDirectory(*directory)) {
        FString myFile = directory + "/" + Filename;
        FFileHelper::LoadFileToString(result, *myFile);
    }
    return result;
}

