
#include "FurniLife_Android.h"
#include "ProceduralMeshComponent.h"
#include "ImagePlateComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidPermissionFunctionLibrary.h"
#endif

AFurniLife_Android* AFurniLife_Android::CurrentInstance = nullptr;

AFurniLife_Android::AFurniLife_Android()
{
    PrimaryActorTick.bCanEverTick = true;
    
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(rootComp);
    
    RoomMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoomMesh"));
    RoomMesh->SetupAttachment(rootComp);
    
#if PLATFORM_ANDROID
    CameraSession = nullptr;
    TFLiteModel = nullptr;
    TFLiteInterpreter = nullptr;
    bIsCameraActive = false;
#endif
}

void AFurniLife_Android::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("FurniLife_Android BeginPlay"));
    AFurniLife_Android::CurrentInstance = this;
    
    // Add lighting
    bool bHasDirectionalLight = false;
    bool bHasSkyLight = false;
    
    for (TActorIterator<ADirectionalLight> DirIt(GetWorld()); DirIt; ++DirIt)
    {
        bHasDirectionalLight = true;
        break;
    }
    
    for (TActorIterator<ASkyLight> SkyIt(GetWorld()); SkyIt; ++SkyIt)
    {
        bHasSkyLight = true;
        break;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    if (!bHasSkyLight)
    {
        ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), SpawnParams);
        if (SkyLight && SkyLight->GetLightComponent())
        {
            SkyLight->GetLightComponent()->SetIntensity(1.0f);
            UE_LOG(LogTemp, Warning, TEXT("Added SkyLight"));
        }
    }
    
    if (!bHasDirectionalLight)
    {
        ADirectionalLight* DirLight = GetWorld()->SpawnActor<ADirectionalLight>(
            ADirectionalLight::StaticClass(),
            FVector::ZeroVector,
            FRotator(-45.0f, 45.0f, 0.0f),
            SpawnParams);
        
        if (DirLight && DirLight->GetLightComponent())
        {
            DirLight->GetLightComponent()->SetIntensity(2.0f);
            UE_LOG(LogTemp, Warning, TEXT("Added DirectionalLight"));
        }
    }
    
#if PLATFORM_ANDROID
    RequestCameraPermission();
    InitializeTFLite();
#endif
    
    if (!DefaultOBJFile.IsEmpty())
    {
        LoadRoomMeshFromFile(DefaultOBJFile);
    }
    
    CreateMenuButton();
}

#if PLATFORM_ANDROID
void AFurniLife_Android::RequestCameraPermission()
{
    TArray<FString> Permissions;
    Permissions.Add(TEXT("android.permission.CAMERA"));
    
    UAndroidPermissionFunctionLibrary::AcquirePermissions(Permissions);
    
    // Check if permission granted
    if (UAndroidPermissionFunctionLibrary::CheckPermission(TEXT("android.permission.CAMERA")))
    {
        InitializeCamera();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Camera permission denied"));
    }
}

void AFurniLife_Android::InitializeCamera()
{
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get JNI environment"));
        return;
    }
    
    // Create camera texture
    CameraTexture = UTexture2D::CreateTransient(640, 480, PF_B8G8R8A8);
    if (CameraTexture)
    {
        CameraTexture->SRGB = true;
        CameraTexture->AddToRoot();
        CameraTexture->UpdateResource();
        
        CameraBuffer.SetNumUninitialized(640 * 480 * 4);
    }
    
    // Create ImagePlate for camera preview
    if (!ImagePlatePost)
    {
        ImagePlatePost = NewObject<UImagePlateComponent>(this, TEXT("CameraPlate"));
        ImagePlatePost->SetupAttachment(RootComponent);
        ImagePlatePost->RegisterComponent();
        
        ImagePlatePost->SetWorldLocation(FVector(300, 0, 100));
        ImagePlatePost->SetRelativeScale3D(FVector(4.0f, 3.0f, 1.0f));
        
        if (CameraTexture)
        {
            UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Engine/EngineMaterials/UnlitMaterial"));
            
            if (BaseMaterial)
            {
                UMaterialInstanceDynamic* CameraMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                CameraMat->SetTextureParameterValue(FName("Texture"), CameraTexture);
                ImagePlatePost->SetMaterial(0, CameraMat);
            }
        }
    }
    
    bIsCameraActive = true;
    UE_LOG(LogTemp, Warning, TEXT("Android camera initialized"));
}

void AFurniLife_Android::InitializeTFLite()
{
    // TensorFlow Lite initialization would go here
    // This requires the TFLite library to be included in the project
    UE_LOG(LogTemp, Warning, TEXT("TFLite initialization placeholder"));
}

void AFurniLife_Android::ProcessCameraFrame()
{
    if (!bIsCameraActive || !CameraTexture) return;
    
    // In a real implementation, get camera frame through JNI
    // For now, just create a test pattern
    FTexture2DMipMap& Mip = CameraTexture->GetPlatformData()->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    
    if (Data)
    {
        uint8* DestPtr = (uint8*)Data;
        for (int32 y = 0; y < 480; y++)
        {
            for (int32 x = 0; x < 640; x++)
            {
                // Create test pattern
                *DestPtr++ = (x % 256); // B
                *DestPtr++ = (y % 256); // G
                *DestPtr++ = 128;       // R
                *DestPtr++ = 255;       // A
            }
        }
        
        Mip.BulkData.Unlock();
        CameraTexture->UpdateResource();
    }
}

void AFurniLife_Android::RunSegmentation()
{
    // TensorFlow Lite segmentation would go here
    UE_LOG(LogTemp, VeryVerbose, TEXT("Running TFLite segmentation"));
}
#endif

void AFurniLife_Android::LoadRoomMeshFromFile(const FString& OBJFileName)
{
    // This is identical to iOS version, so you could extract to a shared function
    TArray<FString> PossiblePaths = {
        FPaths::ProjectSavedDir() / TEXT("ExportedMeshes") / (OBJFileName + TEXT(".obj")),
        FPaths::ProjectContentDir() / TEXT("Models") / (OBJFileName + TEXT(".obj"))
    };
    
    FString FilePath;
    for (const FString& Path : PossiblePaths)
    {
        if (FPaths::FileExists(Path))
        {
            FilePath = Path;
            UE_LOG(LogTemp, Warning, TEXT("Found OBJ file at: %s"), *FilePath);
            break;
        }
    }
    
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("OBJ file '%s' not found"), *OBJFileName);
        return;
    }
    
    // Rest of mesh loading code (same as iOS version)
    // ... (abbreviated for space)
}

void AFurniLife_Android::CreateMenuButton()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;
    
    UClass* WidgetClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/UI/WBP_MenuButton.WBP_MenuButton_C"));
    
    if (WidgetClass)
    {
        MenuWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport(1000);
            
            UButton* MidasButton = Cast<UButton>(MenuWidget->GetWidgetFromName(TEXT("MidasButton")));
            if (MidasButton)
            {
                MidasButton->OnClicked.AddDynamic(this, &AFurniLife_Android::OnMenuButtonClicked);
                UE_LOG(LogTemp, Warning, TEXT("Menu button bound"));
            }
        }
    }
}

void AFurniLife_Android::OnMenuButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Menu button clicked - Loading MidasLevel_Android"));
    UGameplayStatics::OpenLevel(GetWorld(), TEXT("MidasLevel_Android"));
}

void AFurniLife_Android::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
#if PLATFORM_ANDROID
    if (bIsCameraActive)
    {
        ProcessCameraFrame();
        
        static float SegmentationTimer = 0;
        SegmentationTimer += DeltaTime;
        if (SegmentationTimer >= 0.1f) // Run at 10 FPS
        {
            SegmentationTimer = 0;
            RunSegmentation();
        }
    }
#endif
}
