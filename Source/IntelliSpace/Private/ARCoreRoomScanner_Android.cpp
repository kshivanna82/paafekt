#include "ARCoreRoomScanner_Android.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "KismetProceduralMeshLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#endif

AARCoreRoomScanner_Android::AARCoreRoomScanner_Android()
{
    PrimaryActorTick.bCanEverTick = true;
    
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    RootComponent = ProceduralMesh;
    
    bIsScanning = false;
    ARSession = nullptr;
}

void AARCoreRoomScanner_Android::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("=== ARCoreRoomScanner_Android BeginPlay ==="));
    
#if PLATFORM_ANDROID
    InitializeARCore();
#else
    UE_LOG(LogTemp, Warning, TEXT("ARCore only works on Android platform"));
#endif
    
    CreateHUD();
}

void AARCoreRoomScanner_Android::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
#if PLATFORM_ANDROID
    if (bIsScanning)
    {
        static float UpdateTimer = 0;
        UpdateTimer += DeltaTime;
        
        if (UpdateTimer >= 1.0f) // Update every second
        {
            UpdateTimer = 0;
            UpdatePlanesFromARCore();
        }
    }
#endif
}

void AARCoreRoomScanner_Android::StartScanning()
{
    UE_LOG(LogTemp, Warning, TEXT("=== StartScanning called ==="));
    
#if PLATFORM_ANDROID
    if (bIsScanning)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already scanning"));
        return;
    }
    
    bIsScanning = true;
    
    // Clear existing mesh
    MeshVertices.Empty();
    MeshTriangles.Empty();
    MeshNormals.Empty();
    
    UE_LOG(LogTemp, Warning, TEXT("ARCore scanning started"));
#else
    // For testing in editor - create mock data
    CreateMockRoomData();
    bIsScanning = true;
#endif
}

void AARCoreRoomScanner_Android::StopScanning()
{
    UE_LOG(LogTemp, Warning, TEXT("=== StopScanning called ==="));
    
    if (!bIsScanning)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not currently scanning"));
        return;
    }
    
    bIsScanning = false;
    
    UE_LOG(LogTemp, Warning, TEXT("Scanning stopped. Vertices: %d, Triangles: %d"),
        MeshVertices.Num(), MeshTriangles.Num() / 3);
}

void AARCoreRoomScanner_Android::ExportMesh(const FString& FileName)
{
    UE_LOG(LogTemp, Warning, TEXT("=== ExportMesh called with filename: %s ==="), *FileName);
    
    if (MeshVertices.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No mesh data to export"));
        return;
    }
    
    FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("ExportedMeshes");
    FString FilePath = SaveDirectory / (FileName + TEXT(".obj"));
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*SaveDirectory);
    
    FString ObjContent;
    
    // Write vertices
    for (const FVector& Vertex : MeshVertices)
    {
        ObjContent += FString::Printf(TEXT("v %f %f %f\n"), Vertex.X, Vertex.Y, Vertex.Z);
    }
    
    // Write normals
    for (const FVector& Normal : MeshNormals)
    {
        ObjContent += FString::Printf(TEXT("vn %f %f %f\n"), Normal.X, Normal.Y, Normal.Z);
    }
    
    // Write faces
    for (int32 i = 0; i < MeshTriangles.Num(); i += 3)
    {
        ObjContent += FString::Printf(TEXT("f %d//%d %d//%d %d//%d\n"),
            MeshTriangles[i] + 1, MeshTriangles[i] + 1,
            MeshTriangles[i + 1] + 1, MeshTriangles[i + 1] + 1,
            MeshTriangles[i + 2] + 1, MeshTriangles[i + 2] + 1);
    }
    
    FFileHelper::SaveStringToFile(ObjContent, *FilePath);
    
    UE_LOG(LogTemp, Warning, TEXT("Mesh exported to: %s"), *FilePath);
    UE_LOG(LogTemp, Warning, TEXT("   Vertices: %d, Triangles: %d"),
        MeshVertices.Num(), MeshTriangles.Num() / 3);
}

void AARCoreRoomScanner_Android::CreateHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("=== CreateHUD called ==="));
    
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("No PlayerController found"));
        return;
    }
    
    if (!HUDWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("HUDWidgetClass not set - Set BP_ScannerHUD in actor details"));
        return;
    }
    
    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
    if (Widget)
    {
        Widget->AddToViewport();
        
        // Bind Furni Match button
        UButton* FurniMatchButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Button_0")));
        if (FurniMatchButton)
        {
            FurniMatchButton->OnClicked.AddDynamic(this, &AARCoreRoomScanner_Android::OnFurniMatchClicked);
            UE_LOG(LogTemp, Warning, TEXT("Furni Match button bound"));
        }
        
        // Bind Start button
        UButton* StartButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Start")));
        if (StartButton)
        {
            StartButton->OnClicked.AddDynamic(this, &AARCoreRoomScanner_Android::OnStartClicked);
        }
        
        // Bind Stop button
        UButton* StopButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Stop")));
        if (StopButton)
        {
            StopButton->OnClicked.AddDynamic(this, &AARCoreRoomScanner_Android::OnStopClicked);
        }
        
        // Bind Export button
        UButton* ExportButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("Export")));
        if (ExportButton)
        {
            ExportButton->OnClicked.AddDynamic(this, &AARCoreRoomScanner_Android::OnExportClicked);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("HUD created and buttons bound"));
    }
}

void AARCoreRoomScanner_Android::OnFurniMatchClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Furni Match clicked - Loading IntelliSpaceMap_Android ==="));
    
    if (bIsScanning)
    {
        StopScanning();
    }
    
    UGameplayStatics::OpenLevel(GetWorld(), TEXT("IntelliSpaceMap_Android"));
}

#if PLATFORM_ANDROID
void AARCoreRoomScanner_Android::InitializeARCore()
{
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get JNI environment"));
        return;
    }
    
    // Note: Full ARCore integration requires:
    // 1. ARCore SDK for Unreal
    // 2. Java-side implementation
    // 3. Proper JNI bindings
    
    // For now, we'll simulate plane detection
    UE_LOG(LogTemp, Warning, TEXT("ARCore initialized (simulated)"));
}

void AARCoreRoomScanner_Android::UpdatePlanesFromARCore()
{
    // Simulate plane detection by creating a simple room structure
    // In production, this would query actual ARCore planes
    
    if (MeshVertices.Num() == 0) // First time - create floor
    {
        float RoomSize = 300.0f;
        float WallHeight = 250.0f;
        
        // Floor plane
        int32 BaseIndex = MeshVertices.Num();
        MeshVertices.Add(FVector(-RoomSize, -RoomSize, 0));
        MeshVertices.Add(FVector(RoomSize, -RoomSize, 0));
        MeshVertices.Add(FVector(RoomSize, RoomSize, 0));
        MeshVertices.Add(FVector(-RoomSize, RoomSize, 0));
        
        // Floor triangles
        MeshTriangles.Add(BaseIndex + 0);
        MeshTriangles.Add(BaseIndex + 1);
        MeshTriangles.Add(BaseIndex + 2);
        
        MeshTriangles.Add(BaseIndex + 0);
        MeshTriangles.Add(BaseIndex + 2);
        MeshTriangles.Add(BaseIndex + 3);
        
        // Floor normals (pointing up)
        for (int i = 0; i < 4; i++)
        {
            MeshNormals.Add(FVector(0, 0, 1));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Added floor plane"));
    }
    else if (MeshVertices.Num() == 4) // Second update - add walls
    {
        float RoomSize = 300.0f;
        float WallHeight = 250.0f;
        
        // Front wall
        int32 BaseIndex = MeshVertices.Num();
        MeshVertices.Add(FVector(-RoomSize, -RoomSize, 0));
        MeshVertices.Add(FVector(RoomSize, -RoomSize, 0));
        MeshVertices.Add(FVector(RoomSize, -RoomSize, WallHeight));
        MeshVertices.Add(FVector(-RoomSize, -RoomSize, WallHeight));
        
        MeshTriangles.Add(BaseIndex + 0);
        MeshTriangles.Add(BaseIndex + 1);
        MeshTriangles.Add(BaseIndex + 2);
        
        MeshTriangles.Add(BaseIndex + 0);
        MeshTriangles.Add(BaseIndex + 2);
        MeshTriangles.Add(BaseIndex + 3);
        
        // Wall normals (pointing inward)
        for (int i = 0; i < 4; i++)
        {
            MeshNormals.Add(FVector(0, 1, 0));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Added wall planes"));
    }
    
    // Update the procedural mesh
    if (ProceduralMesh && MeshVertices.Num() > 0)
    {
        ProceduralMesh->ClearAllMeshSections();
        
        TArray<FVector2D> UV0;
        TArray<FColor> VertexColors;
        TArray<FProcMeshTangent> Tangents;
        
        for (int32 i = 0; i < MeshVertices.Num(); i++)
        {
            UV0.Add(FVector2D(0, 0));
            VertexColors.Add(FColor::White);
        }
        
        ProceduralMesh->CreateMeshSection(
            0,
            MeshVertices,
            MeshTriangles,
            MeshNormals,
            UV0,
            VertexColors,
            Tangents,
            true
        );
        
        // Apply material
        UMaterial* Material = LoadObject<UMaterial>(nullptr,
            TEXT("/Engine/EngineMaterials/WorldGridMaterial"));
        
        if (Material)
        {
            UMaterialInstanceDynamic* MeshMaterial = UMaterialInstanceDynamic::Create(Material, this);
            MeshMaterial->TwoSided = true;
            ProceduralMesh->SetMaterial(0, MeshMaterial);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Mesh updated: %d vertices, %d triangles"),
            MeshVertices.Num(), MeshTriangles.Num() / 3);
    }
}
#endif

void AARCoreRoomScanner_Android::CreateMockRoomData()
{
    // This creates a simple box room for testing
    float Width = 400.0f;
    float Depth = 400.0f;
    float Height = 300.0f;
    
    // Floor
    MeshVertices.Add(FVector(-Width/2, -Depth/2, 0));
    MeshVertices.Add(FVector(Width/2, -Depth/2, 0));
    MeshVertices.Add(FVector(Width/2, Depth/2, 0));
    MeshVertices.Add(FVector(-Width/2, Depth/2, 0));
    
    for (int i = 0; i < 4; i++)
        MeshNormals.Add(FVector(0, 0, 1));
    
    MeshTriangles.Add(0); MeshTriangles.Add(1); MeshTriangles.Add(2);
    MeshTriangles.Add(0); MeshTriangles.Add(2); MeshTriangles.Add(3);
    
    UE_LOG(LogTemp, Warning, TEXT("Created mock room data for testing"));
}
