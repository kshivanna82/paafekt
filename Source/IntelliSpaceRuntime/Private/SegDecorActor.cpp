#include "SegDecorActor.h"
#include "PlatformCameraCaptureBridge.h"
#include "U2NetRunnerBridge.h"
#include "ModelRepositorySubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "ISLog.h"

static void* GU2Net=nullptr;
static void OnSegFrameThunk(void* User, void* Px){ ((ASegDecorActor*)User)->OnCameraFrame(Px); }

ASegDecorActor::ASegDecorActor(){
    PrimaryActorTick.bCanEverTick=false;
    RoomMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoomMesh"));
    RootComponent=RoomMesh;
}

void ASegDecorActor::BeginPlay(){
    Super::BeginPlay();
    if (SegCompositeMaterial){ SegMID=UMaterialInstanceDynamic::Create(SegCompositeMaterial,this); RoomMesh->SetMaterial(0,SegMID); }

#if PLATFORM_IOS
    if(!GU2Net){ GU2Net=CreateU2NetRunner(); bool ok=GU2Net && LoadU2NetCompiled(GU2Net); if(!ok) ok=GU2Net && LoadU2NetPackage(GU2Net); UE_LOG(LogIntelliSpace, Log, TEXT("[SegDecor] U2Net ready=%d"), ok); }
#else
    UE_LOG(LogIntelliSpace, Warning, TEXT("[SegDecor] U2Net not available on this platform."));
#endif
    RegisterCameraHandler(this,&OnSegFrameThunk); StartCameraCapture();
    SetRoomPreset(CurrentPreset);

    if (RightDrawerWidgetClass){
        RightDrawerWidget=CreateWidget<UUserWidget>(GetWorld(),RightDrawerWidgetClass);
        RightDrawerWidget->AddToViewport(50);
        UE_LOG(LogIntelliSpace, Log, TEXT("[SegDecor] Right drawer created."));
    }
}
void ASegDecorActor::EndPlay(const EEndPlayReason::Type R){ StopCameraCapture(); Super::EndPlay(R); }

void ASegDecorActor::OnCameraFrame(void* PixelBuffer){
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,[this,PixelBuffer](){ UpdateSegmentation(PixelBuffer); });
}

void ASegDecorActor::UpdateSegmentation(void* PixelBuffer){
#if PLATFORM_IOS
    if(!GU2Net) return; int W=0,H=0; TArray<float> Mask; Mask.SetNumUninitialized(320*320);
    if(!RunU2Net(GU2Net,(CVPixelBufferRef)PixelBuffer,Mask.GetData(),&W,&H) || W<=0||H<=0) { UE_LOG(LogIntelliSpace, Warning, TEXT("[SegDecor] Segmentation failed.")); return; }
    Mask.SetNumUninitialized(W*H,false);
    AsyncTask(ENamedThreads::GameThread,[this,Mask=MoveTemp(Mask),W,H]() mutable { ApplyMaskToMaterial(Mask,W,H); });
#else
    UE_LOG(LogIntelliSpace, VeryVerbose, TEXT("[SegDecor] UpdateSegmentation skipped (platform not supported)."));
#endif
}

void ASegDecorActor::ApplyMaskToMaterial(const TArray<float>& Mask,int W,int H){
    if(!SegMID) return;
    static UTexture2D* MaskTex=nullptr;
    if(!MaskTex){ MaskTex=UTexture2D::CreateTransient(W,H,PF_G8); MaskTex->SRGB=false; MaskTex->UpdateResource(); }
    FUpdateTextureRegion2D R(0,0,0,0,W,H);
    TArray<uint8> Bytes; Bytes.SetNumUninitialized(W*H);
    for(int i=0;i<W*H;++i) Bytes[i]=(uint8)FMath::Clamp(Mask[i]*255.f,0.f,255.f);
    MaskTex->UpdateTextureRegions(0,1,&R,W*sizeof(uint8),sizeof(uint8),Bytes.GetData());
    SegMID->SetTextureParameterValue(TEXT("MaskTex"),MaskTex);
    UE_LOG(LogIntelliSpace, Verbose, TEXT("[SegDecor] Mask applied (%dx%d)"), W, H);
}

void ASegDecorActor::SetRoomPreset(ERoomPreset Preset){
    CurrentPreset=Preset;
    UE_LOG(LogIntelliSpace, Log, TEXT("[SegDecor] Preset -> %d"), (int)Preset);
}

void ASegDecorActor::LaunchBuilderMode(){
    UE_LOG(LogIntelliSpace, Log, TEXT("[SegDecor] LaunchBuilderMode (implement level switch/spawn)."));
}
