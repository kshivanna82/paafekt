#include "ISBuilderActor.h"
#include "Async/Async.h"
#include "MeshGenerator.h"
#include "MiDaSRunnerBridge.h"
#include "PlatformCameraCaptureBridge.h"
#include "IOSShareBridge.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Components/Button.h"
#include "ISLog.h"
#include <vector>

static void* GMiDaSRunner = nullptr;
static void OnCameraFrameThunk(void* User, void* Pixel) { ((AISBuilderActor*)User)->OnCameraFrameFromPixelBuffer(Pixel); }

AISBuilderActor::AISBuilderActor(){
    PrimaryActorTick.bCanEverTick = true;
    RoomMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoomMesh"));
    RootComponent = RoomMesh;
}

void AISBuilderActor::BeginPlay(){
    Super::BeginPlay();
#if PLATFORM_IOS
    if(!GMiDaSRunner){
        GMiDaSRunner=CreateMiDaSRunner(); bool ok=GMiDaSRunner && LoadMiDaSCompiled(GMiDaSRunner);
        if(!ok) ok=GMiDaSRunner && LoadMiDaSPackage(GMiDaSRunner);
        UE_LOG(LogIntelliSpace, Log, TEXT("[Builder] MiDaS loaded=%d"), ok);
        if(!ok) UE_LOG(LogIntelliSpace, Error, TEXT("[Builder] MiDaS init failed"));
    }
#else
    UE_LOG(LogIntelliSpace, Warning, TEXT("[Builder] MiDaS not available on this platform."));
#endif
    SessionFrameCount=0; SessionStartTime=FPlatformTime::Seconds(); StartCamera();
}

void AISBuilderActor::EndPlay(const EEndPlayReason::Type R){
    StopCamera();
#if PLATFORM_IOS
    if(LastFrame){ /* released on replace */ }
    if(GMiDaSRunner){ DestroyMiDaSRunner(GMiDaSRunner); GMiDaSRunner=nullptr; }
#endif
    Super::EndPlay(R);
}
void AISBuilderActor::Tick(float){}

void AISBuilderActor::DiscardCurrentCapture(){
    StopCamera();
#if PLATFORM_IOS
    if(LastFrame){ CVPixelBufferRelease((CVPixelBufferRef)LastFrame); LastFrame=nullptr; }
#endif
    LastVerts.Reset(); LastTris.Reset();
    SessionFrameCount = 0; FrameCounter = 0; State = ECaptureState::Capturing;
    UE_LOG(LogIntelliSpace, Log, TEXT("[Builder] Discarded current capture."));
}

void AISBuilderActor::OnCameraFrameFromPixelBuffer(void* PixelBuffer){
    if (State != ECaptureState::Capturing) return;
    if (++FrameCounter % FrameInterval != 0) return;
#if PLATFORM_IOS
    CVPixelBufferRef Px=(CVPixelBufferRef)PixelBuffer;
    CVPixelBufferRetain(Px);
    if(LastFrame) { CVPixelBufferRef Old = (CVPixelBufferRef)LastFrame; CVPixelBufferRelease(Old); }
    LastFrame = Px;
#endif
    SessionFrameCount++; const double elapsed = FPlatformTime::Seconds() - SessionStartTime;
    if (SessionFrameCount >= TargetFrameCount || elapsed >= TargetSeconds) {
        StopCamera(); State = ECaptureState::WaitingConfirm; ShowScanPrompt();
        UE_LOG(LogIntelliSpace, Log, TEXT("[Builder] Prompt after %d frames / %.2fs."), SessionFrameCount, elapsed);
        return;
    }
}

void AISBuilderActor::UpdateDepthAndMesh(void* InputFrame){
#if PLATFORM_IOS
    if(!GMiDaSRunner || !InputFrame){ State=ECaptureState::Capturing; return; }
    constexpr int W=256, H=256; std::vector<float> Depth(W*H,0.f);
    if(!RunMiDaS(GMiDaSRunner,(CVPixelBufferRef)InputFrame,Depth.data())){ State=ECaptureState::Capturing; UE_LOG(LogIntelliSpace, Warning, TEXT("[Builder] MiDaS inference failed")); return; }
    TArray<FVector> V; TArray<int32> T;
    if(!BuildMeshFromDepthMap(Depth,W,H,V,T,100.f,2)){ State=ECaptureState::Capturing; UE_LOG(LogIntelliSpace, Warning, TEXT("[Builder] Mesh build failed")); return; }
    AsyncTask(ENamedThreads::GameThread,[this,V=MoveTemp(V),T=MoveTemp(T)]() mutable {
        RoomMesh->CreateMeshSection(0,V,T,{}, {}, {}, {}, true); LastVerts=V; LastTris=T;
        UE_LOG(LogIntelliSpace, Log, TEXT("[Builder] Depth OK -> verts=%d, tris=%d"), V.Num(), T.Num()/3);
        ShowReviewUI(); State=ECaptureState::Reviewing;
    });
#else
    UE_LOG(LogIntelliSpace, VeryVerbose, TEXT("[Builder] UpdateDepthAndMesh skipped (platform not supported)."));
#endif
}

void AISBuilderActor::ShowScanPrompt(){
    if (ScanPromptWidget || !ScanPromptWidgetClass) return;
    ScanPromptWidget = CreateWidget<UUserWidget>(GetWorld(), ScanPromptWidgetClass);
    if (!ScanPromptWidget) return;
    if (UButton* b=Cast<UButton>(ScanPromptWidget->GetWidgetFromName(TEXT("BtnBuildNow")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnScanPrompt_BuildNow);
    if (UButton* b=Cast<UButton>(ScanPromptWidget->GetWidgetFromName(TEXT("BtnKeepScanning")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnScanPrompt_KeepScanning);
    if (UButton* b=Cast<UButton>(ScanPromptWidget->GetWidgetFromName(TEXT("BtnReset")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnScanPrompt_Reset);
    if (UButton* b=Cast<UButton>(ScanPromptWidget->GetWidgetFromName(TEXT("BtnDiscard")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnScanPrompt_Discard);
    ScanPromptWidget->AddToViewport(300);
}

void AISBuilderActor::HideScanPrompt(){ if(ScanPromptWidget){ ScanPromptWidget->RemoveFromParent(); ScanPromptWidget=nullptr; } }

void AISBuilderActor::OnScanPrompt_BuildNow(){
    HideScanPrompt();
#if PLATFORM_IOS
    if(LastFrame){
        CVPixelBufferRef Px=(CVPixelBufferRef)LastFrame; CVPixelBufferRetain(Px);
        State=ECaptureState::Processing;
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,[this,Px](){ UpdateDepthAndMesh(Px); CVPixelBufferRelease(Px); });
    } else {
        SessionFrameCount=0; SessionStartTime=FPlatformTime::Seconds(); StartCamera(); State=ECaptureState::Capturing;
    }
#else
    UE_LOG(LogIntelliSpace, Warning, TEXT("[Builder] BuildNow on unsupported platform."));
#endif
}

void AISBuilderActor::OnScanPrompt_KeepScanning(){ HideScanPrompt(); StartCamera(); State=ECaptureState::Capturing; }
void AISBuilderActor::OnScanPrompt_Reset(){
    HideScanPrompt(); SessionFrameCount=0; SessionStartTime=FPlatformTime::Seconds();
#if PLATFORM_IOS
    if(LastFrame){ CVPixelBufferRelease((CVPixelBufferRef)LastFrame); LastFrame=nullptr; }
#endif
    StartCamera(); State=ECaptureState::Capturing;
}
void AISBuilderActor::OnScanPrompt_Discard(){ HideScanPrompt(); DiscardCurrentCapture(); }

void AISBuilderActor::ShowReviewUI(){
    if (ReviewWidget || !ReviewWidgetClass) return;
    ReviewWidget = CreateWidget<UUserWidget>(GetWorld(), ReviewWidgetClass);
    if (!ReviewWidget) return;
    if (UButton* b=Cast<UButton>(ReviewWidget->GetWidgetFromName(TEXT("BtnExport")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnReview_Export);
    if (UButton* b=Cast<UButton>(ReviewWidget->GetWidgetFromName(TEXT("BtnRetake")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnReview_Retake);
    if (UButton* b=Cast<UButton>(ReviewWidget->GetWidgetFromName(TEXT("BtnDiscardReview")))) b->OnClicked.AddDynamic(this,&AISBuilderActor::OnReview_Discard);
    ReviewWidget->AddToViewport(200);
}

void AISBuilderActor::HideReviewUI(){ if(ReviewWidget){ ReviewWidget->RemoveFromParent(); ReviewWidget=nullptr; } }

void AISBuilderActor::OnReview_Export(){
    SaveCurrentMeshOBJ();
    HideReviewUI();
    SessionFrameCount=0; SessionStartTime=FPlatformTime::Seconds(); StartCamera(); State=ECaptureState::Capturing;
}

void AISBuilderActor::OnReview_Retake(){
    HideReviewUI(); SessionFrameCount=0; SessionStartTime=FPlatformTime::Seconds(); StartCamera(); State=ECaptureState::Capturing;
}

void AISBuilderActor::OnReview_Discard(){
    HideReviewUI(); DiscardCurrentCapture();
}

void AISBuilderActor::SaveCurrentMeshOBJ(const FString& Name){
    if(LastVerts.Num()==0||LastTris.Num()==0) return;
    FString Out = TEXT("# MiDaS mesh
");
    for(const FVector& P:LastVerts) Out += FString::Printf(TEXT("v %f %f %f
"),P.X,P.Y,P.Z);
    for(int32 i=0;i+2<LastTris.Num();i+=3) Out += FString::Printf(TEXT("f %d %d %d
"),LastTris[i]+1,LastTris[i+1]+1,LastTris[i+2]+1);
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Meshes")); IFileManager::Get().MakeDirectory(*Dir,true);
    const FString Path=FPaths::Combine(Dir,Name); FFileHelper::SaveStringToFile(Out,*Path);
    UE_LOG(LogIntelliSpace, Log, TEXT("[Builder] Saved OBJ: %s"), *Path);
    IOS_ShowShareSheetForFile(*Path); // iOS share
}

void AISBuilderActor::StartCamera(){
    RegisterCameraHandler(this,&OnCameraFrameThunk);
    if(!StartCameraCapture()) UE_LOG(LogIntelliSpace, Error, TEXT("[Camera] start failed"));
    else UE_LOG(LogIntelliSpace, Log, TEXT("[Camera] started."));
}

void AISBuilderActor::StopCamera(){
    StopCameraCapture();
    UE_LOG(LogIntelliSpace, Log, TEXT("[Camera] stopped."));
}

#if WITH_DEV_AUTOMATION_TESTS
void AISBuilderActor::__Test_BuildDummyAndExport(){
    std::vector<float> D(256*256, 0.f);
    for(int y=0;y<256;++y) for(int x=0;x<256;++x) D[y*256+x]=float(x+y)/512.f;
    TArray<FVector> V; TArray<int32> T;
    if(BuildMeshFromDepthMap(D,256,256,V,T,100.f,4)){
        LastVerts=V; LastTris=T; SaveCurrentMeshOBJ(TEXT("TestMesh.obj"));
    }
}
#endif
