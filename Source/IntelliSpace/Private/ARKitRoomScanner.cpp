// ARKitRoomScanner.cpp

// Class header MUST be first for Unreal
#include "ARKitRoomScanner.h"

// Then undefine Apple macros before OpenCV
#ifdef NO
    #undef NO
#endif
#ifdef YES
    #undef YES
#endif
#ifdef check
    #undef check
#endif

// Now include OpenCV
#include <opencv2/opencv.hpp>

// Then other includes
#include "ProceduralMeshComponent.h"
#include "ImagePlateComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

// iOS headers last
#if PLATFORM_IOS
#import <ARKit/ARKit.h>
#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>

@interface ARKitDelegate : NSObject<ARSessionDelegate>
@property (nonatomic, assign) AARKitRoomScanner* Owner;
@property (nonatomic, strong) NSMutableArray<ARMeshAnchor*>* meshAnchors;
@end

@implementation ARKitDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        self.meshAnchors = [NSMutableArray array];
    }
    return self;
}

- (void)session:(ARSession *)session didAddAnchors:(NSArray<ARAnchor*>*)anchors {
    for (ARAnchor* anchor in anchors) {
        if ([anchor isKindOfClass:[ARMeshAnchor class]]) {
            ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
            [self.meshAnchors addObject:meshAnchor];
            
            if (self.Owner) {
                UE_LOG(LogTemp, Warning, TEXT("ARKit: Mesh anchor added"));
            }
        }
    }
}

- (void)session:(ARSession *)session didUpdateAnchors:(NSArray<ARAnchor*>*)anchors {
    for (ARAnchor* anchor in anchors) {
        if ([anchor isKindOfClass:[ARMeshAnchor class]]) {
            ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
            
            NSUInteger index = [self.meshAnchors indexOfObjectPassingTest:^BOOL(ARMeshAnchor* obj, NSUInteger idx, BOOL* stop) {
                return [obj.identifier isEqual:meshAnchor.identifier];
            }];
            
            if (index != NSNotFound) {
                self.meshAnchors[index] = meshAnchor;
            }
        }
    }
}

- (void)session:(ARSession *)session didRemoveAnchors:(NSArray<ARAnchor*>*)anchors {
    for (ARAnchor* anchor in anchors) {
        if ([anchor isKindOfClass:[ARMeshAnchor class]]) {
            ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
            [self.meshAnchors removeObject:meshAnchor];
        }
    }
}

@end
#endif

AARKitRoomScanner::AARKitRoomScanner()
{
    PrimaryActorTick.bCanEverTick = true;
    
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    RootComponent = ProceduralMesh;
    
    CameraPreviewPlate = CreateDefaultSubobject<UImagePlateComponent>(TEXT("CameraPreview"));
    CameraPreviewPlate->SetupAttachment(RootComponent);
    
    bIsScanning = false;
    ARSession = nullptr;
    ARDelegate = nullptr;
    TimeSinceLastUpdate = 0.0f;
    MeshUpdateInterval = 0.5f;
    
    cameraFrame = new cv::Mat();
    
    UE_LOG(LogTemp, Warning, TEXT("ARKitRoomScanner constructor completed"));
}

AARKitRoomScanner::~AARKitRoomScanner()
{
    delete cameraFrame;
}

void AARKitRoomScanner::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("=== ARKitRoomScanner BeginPlay START ==="));
    
    // Setup camera preview texture
    CameraTexture = UTexture2D::CreateTransient(640, 480, PF_B8G8R8A8);
    CameraTexture->SRGB = true;
    CameraTexture->AddToRoot();
    CameraTexture->UpdateResource();
    
    CameraColorData.SetNumUninitialized(640 * 480);
    UE_LOG(LogTemp, Warning, TEXT("Camera texture created: %dx%d"), 640, 480);
    
    // Position camera preview
    if (CameraPreviewPlate)
    {
        // Position in front of player, visible area
        CameraPreviewPlate->SetRelativeLocation(FVector(500, 0, 100));
        CameraPreviewPlate->SetRelativeScale3D(FVector(3.0f, 2.0f, 1.0f));
        CameraPreviewPlate->SetVisibility(true);
        CameraPreviewPlate->SetHiddenInGame(false);
        
        CameraMaterial = NewObject<UMaterialInstanceDynamic>(this);
        CameraPreviewPlate->SetMaterial(0, CameraMaterial);
        
        if (CameraMaterial && CameraTexture)
        {
            CameraMaterial->SetTextureParameterValue(FName("Texture"), CameraTexture);
            CameraMaterial->SetTextureParameterValue(FName("BaseTexture"), CameraTexture);
            UE_LOG(LogTemp, Warning, TEXT("✅ Camera preview material set"));
        }
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Camera preview plate configured at location: %s"),
            *CameraPreviewPlate->GetRelativeLocation().ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ CameraPreviewPlate is NULL"));
    }
    
#if PLATFORM_IOS
    UE_LOG(LogTemp, Warning, TEXT("Platform: iOS - Initializing ARKit"));
    
    if (ARWorldTrackingConfiguration.isSupported) {
        id sessionObj = [[NSClassFromString(@"ARSession") alloc] init];
        ARSession = (void*)sessionObj;
        
        ARKitDelegate* delegate = [[ARKitDelegate alloc] init];
        delegate.Owner = this;
        [(id)ARSession setDelegate:delegate];
        ARDelegate = (void*)delegate;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ ARKit initialized"));
        
        if ([ARWorldTrackingConfiguration supportsSceneReconstruction:ARSceneReconstructionMesh]) {
            UE_LOG(LogTemp, Warning, TEXT("✅ Scene reconstruction supported (LiDAR detected)"));
        } else {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ Scene reconstruction not supported (no LiDAR)"));
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("❌ ARKit not supported on this device"));
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("Platform: NOT iOS - ARKit unavailable"));
#endif
    
    CreateHUD();
    
    UE_LOG(LogTemp, Warning, TEXT("=== ARKitRoomScanner BeginPlay END ==="));
}

void AARKitRoomScanner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("ARKitRoomScanner EndPlay"));
    
#if PLATFORM_IOS
    if (ARSession) {
        [(id)ARSession pause];
        ARSession = nullptr;
    }
    
    if (ARDelegate) {
        [(ARKitDelegate*)ARDelegate setOwner:nullptr];
        ARDelegate = nullptr;
    }
#endif
    
    Super::EndPlay(EndPlayReason);
}

void AARKitRoomScanner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsScanning)
    {
        UpdateCameraFrame();
        
        TimeSinceLastUpdate += DeltaTime;
        if (TimeSinceLastUpdate >= MeshUpdateInterval)
        {
            TimeSinceLastUpdate = 0.0f;
            UpdateMeshFromARKit();
        }
    }
}

void AARKitRoomScanner::StartScanning()
{
    UE_LOG(LogTemp, Warning, TEXT("=== StartScanning called ==="));
    
#if PLATFORM_IOS
    if (!ARSession)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ARSession is NULL"));
        return;
    }
    
    if (bIsScanning)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already scanning"));
        return;
    }
    
    ARWorldTrackingConfiguration* config = [[ARWorldTrackingConfiguration alloc] init];
    
    if ([ARWorldTrackingConfiguration supportsSceneReconstruction:ARSceneReconstructionMesh]) {
        config.sceneReconstruction = ARSceneReconstructionMesh;
        UE_LOG(LogTemp, Warning, TEXT("Scene reconstruction enabled"));
    }
    
    config.planeDetection = ARPlaneDetectionHorizontal | ARPlaneDetectionVertical;
    config.environmentTexturing = AREnvironmentTexturingAutomatic;
    config.lightEstimationEnabled = true;
    
    [(id)ARSession runWithConfiguration:config];
    bIsScanning = true;
    
    // Make sure camera preview is visible
    if (CameraPreviewPlate)
    {
        CameraPreviewPlate->SetVisibility(true);
        CameraPreviewPlate->SetHiddenInGame(false);
        UE_LOG(LogTemp, Warning, TEXT("Camera preview plate visibility set to true"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ ARKit scanning started successfully"));
#else
    UE_LOG(LogTemp, Warning, TEXT("StartScanning called but not on iOS"));
    bIsScanning = true; // For testing in editor
#endif
}

void AARKitRoomScanner::StopScanning()
{
    UE_LOG(LogTemp, Warning, TEXT("=== StopScanning called ==="));
    
#if PLATFORM_IOS
    if (!ARSession)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ARSession is NULL"));
        return;
    }
    
    if (!bIsScanning)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not currently scanning"));
        return;
    }
    
    [(id)ARSession pause];
    bIsScanning = false;
    
    UE_LOG(LogTemp, Warning, TEXT("✅ ARKit scanning stopped. Vertices: %d, Triangles: %d"),
        CurrentMeshData.VertexCount, CurrentMeshData.TriangleCount);
#else
    UE_LOG(LogTemp, Warning, TEXT("StopScanning called but not on iOS"));
    bIsScanning = false;
#endif
}

void AARKitRoomScanner::ToggleScanning()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ToggleScanning called ==="));
    
    if (bIsScanning) {
        StopScanning();
    } else {
        StartScanning();
    }
}

void AARKitRoomScanner::ClearMesh()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ClearMesh called ==="));
    
    if (ProceduralMesh)
    {
        ProceduralMesh->ClearAllMeshSections();
        CurrentMeshData.Vertices.Empty();
        CurrentMeshData.Triangles.Empty();
        CurrentMeshData.Normals.Empty();
        CurrentMeshData.VertexCount = 0;
        CurrentMeshData.TriangleCount = 0;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Mesh cleared"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ProceduralMesh is NULL"));
    }
}

void AARKitRoomScanner::UpdateCameraFrame()
{
#if PLATFORM_IOS
    if (!ARSession) return;
    
    id sessionObj = (id)ARSession;
    ARFrame* currentFrame = [sessionObj currentFrame];
    
    if (currentFrame && currentFrame.capturedImage)
    {
        // Log occasionally to avoid spam
        static int frameCount = 0;
        if (frameCount++ % 30 == 0) // Log every 30 frames
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("Camera frame updating (frame %d)"), frameCount);
        }
        
        CVPixelBufferRef pixelBuffer = currentFrame.capturedImage;
        
        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        
        size_t width = CVPixelBufferGetWidth(pixelBuffer);
        size_t height = CVPixelBufferGetHeight(pixelBuffer);
        
        void* yPlane = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
        size_t yBytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        
        if (CameraTexture && yPlane)
        {
            if (CameraTexture->GetSizeX() != width || CameraTexture->GetSizeY() != height)
            {
                CameraTexture->RemoveFromRoot();
                CameraTexture = UTexture2D::CreateTransient(width, height, PF_B8G8R8A8);
                CameraTexture->SRGB = true;
                CameraTexture->AddToRoot();
                CameraColorData.SetNumUninitialized(width * height);
                
                if (CameraMaterial)
                {
                    CameraMaterial->SetTextureParameterValue(FName("Texture"), CameraTexture);
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Camera texture resized to %dx%d"), width, height);
            }
            
            FTexture2DMipMap& Mip = CameraTexture->GetPlatformData()->Mips[0];
            void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
            
            if (Data)
            {
                uint8* DestPtr = (uint8*)Data;
                uint8* SrcPtr = (uint8*)yPlane;
                
                for (size_t y = 0; y < height; y++)
                {
                    for (size_t x = 0; x < width; x++)
                    {
                        uint8 luma = SrcPtr[y * yBytesPerRow + x];
                        
                        *DestPtr++ = luma; // B
                        *DestPtr++ = luma; // G
                        *DestPtr++ = luma; // R
                        *DestPtr++ = 255;  // A
                    }
                }
                
                Mip.BulkData.Unlock();
                CameraTexture->UpdateResource();
            }
        }
        
        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    }
#endif
}

void AARKitRoomScanner::UpdateMeshFromARKit()
{
#if PLATFORM_IOS
    if (!ARDelegate) return;
    
    ARKitDelegate* delegate = (ARKitDelegate*)ARDelegate;
    
    CurrentMeshData.Vertices.Empty();
    CurrentMeshData.Triangles.Empty();
    CurrentMeshData.Normals.Empty();
    
    int32 vertexOffset = 0;
    int32 meshAnchorCount = delegate.meshAnchors.count;
    
    if (meshAnchorCount > 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Processing %d mesh anchors"), meshAnchorCount);
    }
    
    for (ARMeshAnchor* meshAnchor in delegate.meshAnchors) {
        ARMeshGeometry* geometry = meshAnchor.geometry;
        
        if (!geometry) continue;
        
        // Get vertex data from Metal buffer
        ARGeometrySource* vertexSource = geometry.vertices;
        NSInteger vertexCount = vertexSource.count;
        id<MTLBuffer> vertexBuffer = vertexSource.buffer;
        const simd_float3* vertices = (const simd_float3*)[vertexBuffer contents];
        
        // Get face data from Metal buffer
        ARGeometryElement* faceSource = geometry.faces;
        NSInteger faceCount = faceSource.count;
        id<MTLBuffer> faceBuffer = faceSource.buffer;
        
        // Handle different index types
        const void* faceBytes = [faceBuffer contents];
        
        // Get normals if available
        const simd_float3* normals = nullptr;
        if (geometry.normals) {
            ARGeometrySource* normalSource = geometry.normals;
            id<MTLBuffer> normalBuffer = normalSource.buffer;
            normals = (const simd_float3*)[normalBuffer contents];
        }
        
        // Transform matrix
        simd_float4x4 transform = meshAnchor.transform;
        
        // Add vertices
        for (NSInteger i = 0; i < vertexCount; i++) {
            simd_float3 vertex = vertices[i];
            
            // Proper matrix multiplication
            simd_float4 vertex4 = simd_make_float4(vertex.x, vertex.y, vertex.z, 1.0f);
            simd_float4 transformedVertex = simd_mul(transform, vertex4);
            
            // Convert to Unreal coordinates
            FVector unrealVertex(
                transformedVertex.x * 100.0f,
                transformedVertex.z * 100.0f,
                transformedVertex.y * 100.0f
            );
            
            CurrentMeshData.Vertices.Add(unrealVertex);
            
            if (normals) {
                simd_float3 normal = normals[i];
                FVector unrealNormal(normal.x, normal.z, normal.y);
                CurrentMeshData.Normals.Add(unrealNormal);
            } else {
                CurrentMeshData.Normals.Add(FVector(0, 0, 1));
            }
        }
        
        // Add triangles based on index size
        if (faceSource.bytesPerIndex == 2) {
            // 16-bit indices
            const uint16_t* indices = (const uint16_t*)faceBytes;
            for (NSInteger i = 0; i < faceCount * 3; i += 3) {
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i]);
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 2]);
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 1]);
            }
        } else if (faceSource.bytesPerIndex == 4) {
            // 32-bit indices
            const uint32_t* indices = (const uint32_t*)faceBytes;
            for (NSInteger i = 0; i < faceCount * 3; i += 3) {
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i]);
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 2]);
                CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 1]);
            }
        }
        
        vertexOffset += vertexCount;
    }
    
    CurrentMeshData.VertexCount = CurrentMeshData.Vertices.Num();
    CurrentMeshData.TriangleCount = CurrentMeshData.Triangles.Num() / 3;
    
    if (ProceduralMesh && CurrentMeshData.Vertices.Num() > 0)
    {
        ProceduralMesh->ClearAllMeshSections();
        
        TArray<FVector2D> UV0;
        TArray<FColor> VertexColors;
        TArray<FProcMeshTangent> Tangents;
        
        for (int32 i = 0; i < CurrentMeshData.Vertices.Num(); i++)
        {
            UV0.Add(FVector2D(0, 0));
            VertexColors.Add(FColor::White);
        }
        
        ProceduralMesh->CreateMeshSection(
            0,
            CurrentMeshData.Vertices,
            CurrentMeshData.Triangles,
            CurrentMeshData.Normals,
            UV0,
            VertexColors,
            Tangents,
            true
        );
        
        if (MeshMaterial)
        {
            ProceduralMesh->SetMaterial(0, MeshMaterial);
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Mesh updated: %d vertices, %d triangles"),
            CurrentMeshData.VertexCount, CurrentMeshData.TriangleCount);
    }
#endif
}

void AARKitRoomScanner::ExportMesh(const FString& FileName)
{
    UE_LOG(LogTemp, Warning, TEXT("=== ExportMesh called with filename: %s ==="), *FileName);
    
    if (CurrentMeshData.Vertices.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ No mesh data to export"));
        return;
    }
    
    FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("ExportedMeshes");
    FString FilePath = SaveDirectory / (FileName + TEXT(".obj"));
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*SaveDirectory);
    
    FString ObjContent;
    
    for (const FVector& Vertex : CurrentMeshData.Vertices)
    {
        ObjContent += FString::Printf(TEXT("v %f %f %f\n"), Vertex.X, Vertex.Y, Vertex.Z);
    }
    
    for (const FVector& Normal : CurrentMeshData.Normals)
    {
        ObjContent += FString::Printf(TEXT("vn %f %f %f\n"), Normal.X, Normal.Y, Normal.Z);
    }
    
    for (int32 i = 0; i < CurrentMeshData.Triangles.Num(); i += 3)
    {
        ObjContent += FString::Printf(TEXT("f %d//%d %d//%d %d//%d\n"),
            CurrentMeshData.Triangles[i] + 1, CurrentMeshData.Triangles[i] + 1,
            CurrentMeshData.Triangles[i + 1] + 1, CurrentMeshData.Triangles[i + 1] + 1,
            CurrentMeshData.Triangles[i + 2] + 1, CurrentMeshData.Triangles[i + 2] + 1);
    }
    
    FFileHelper::SaveStringToFile(ObjContent, *FilePath);
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Mesh exported to: %s"), *FilePath);
    UE_LOG(LogTemp, Warning, TEXT("   Vertices: %d, Triangles: %d"),
        CurrentMeshData.VertexCount, CurrentMeshData.TriangleCount);
}

void AARKitRoomScanner::CreateHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("=== CreateHUD called ==="));
    
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No PlayerController found"));
        return;
    }
    
    if (!HUDWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ HUDWidgetClass is NULL - Set it in the actor details panel!"));
        UE_LOG(LogTemp, Error, TEXT("   1. Select ARKitRoomScanner in the level"));
        UE_LOG(LogTemp, Error, TEXT("   2. In Details panel, find 'UI' section"));
        UE_LOG(LogTemp, Error, TEXT("   3. Set 'HUD Widget Class' to BP_ScannerHUD"));
        return;
    }
    
    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
    if (Widget)
    {
        Widget->AddToViewport();
        UE_LOG(LogTemp, Warning, TEXT("✅ HUD Widget created and added to viewport"));
        UE_LOG(LogTemp, Warning, TEXT("   Widget class: %s"), *HUDWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create widget from class"));
    }
}

FString AARKitRoomScanner::GetStatusText() const
{
    if (bIsScanning) {
        return FString::Printf(TEXT("Scanning: %d vertices"), CurrentMeshData.VertexCount);
    }
    return TEXT("Ready");
}
