#include "ISBuilderActor.h"

AISBuilderActor::AISBuilderActor()
    : AISBuilderActor(FObjectInitializer::Get())
{
}

AISBuilderActor::AISBuilderActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AISBuilderActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Verbose, TEXT("[Builder] BeginPlay"));
}

void AISBuilderActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void AISBuilderActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Verbose, TEXT("[Builder] EndPlay"));
    Super::EndPlay(EndPlayReason);
}

void AISBuilderActor::OnReview_Export()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnReview_Export"));
}

void AISBuilderActor::OnReview_Retake()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnReview_Retake"));
}

void AISBuilderActor::OnReview_Discard()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnReview_Discard"));
}

void AISBuilderActor::OnScanPrompt_Reset()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnScanPrompt_Reset"));
}

void AISBuilderActor::OnScanPrompt_Discard()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnScanPrompt_Discard"));
}

void AISBuilderActor::OnScanPrompt_BuildNow()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnScanPrompt_BuildNow"));
}

void AISBuilderActor::OnScanPrompt_KeepScanning()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] OnScanPrompt_KeepScanning"));
}

void AISBuilderActor::DiscardCurrentCapture()
{
    UE_LOG(LogTemp, Log, TEXT("[Builder] DiscardCurrentCapture"));
}
