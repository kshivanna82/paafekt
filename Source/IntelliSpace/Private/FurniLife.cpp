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
    FString OnnxPath = FPaths::ProjectContentDir() / TEXT("Models/U2Net1.onnx");
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *OnnxPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load ONNX model: %s"), *OnnxPath);
        return;
    }

	
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

