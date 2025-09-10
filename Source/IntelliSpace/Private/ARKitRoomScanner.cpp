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

// Redefine for Objective-C use
#if PLATFORM_IOS
#define YES 1
#define NO 0
#endif

// iOS headers last
#if PLATFORM_IOS
#import <ARKit/ARKit.h>
#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>

@interface ARKitDelegate : NSObject<ARSessionDelegate>
@property (nonatomic, assign) AARKitRoomScanner* Owner;
@property (nonatomic, strong) NSMutableArray<ARMeshAnchor*>* meshAnchors;
@property (nonatomic, strong) NSMutableArray<ARPlaneAnchor*>* planeAnchors;
@property (nonatomic) BOOL usePlanes;
@end

@implementation ARKitDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        self.meshAnchors = [NSMutableArray array];
        self.planeAnchors = [NSMutableArray array];
        self.usePlanes = NO;
    }
    return self;
}

- (void)session:(ARSession *)session didAddAnchors:(NSArray<ARAnchor*>*)anchors {
    @autoreleasepool {
        for (ARAnchor* anchor in anchors) {
            if ([anchor isKindOfClass:[ARMeshAnchor class]] && !self.usePlanes) {
                ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
                [self.meshAnchors addObject:meshAnchor];
                
                if (self.Owner) {
                    UE_LOG(LogTemp, Warning, TEXT("ARKit: Mesh anchor added"));
                }
            }
            else if ([anchor isKindOfClass:[ARPlaneAnchor class]]) {
                ARPlaneAnchor* planeAnchor = (ARPlaneAnchor*)anchor;
                [self.planeAnchors addObject:planeAnchor];
                
                if (self.Owner) {
                    NSString* alignment = @"Unknown";
                    if (planeAnchor.alignment == ARPlaneAnchorAlignmentHorizontal) {
                        alignment = @"Horizontal";
                    } else if (planeAnchor.alignment == ARPlaneAnchorAlignmentVertical) {
                        alignment = @"Vertical";
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("ARKit: Plane anchor added - %s, Size: %.2f x %.2f"),
                        *FString(alignment),
                        planeAnchor.extent.x,
                        planeAnchor.extent.z);
                }
            }
        }
    }
}

- (void)session:(ARSession *)session didUpdateAnchors:(NSArray<ARAnchor*>*)anchors {
    @autoreleasepool {
        for (ARAnchor* anchor in anchors) {
            if ([anchor isKindOfClass:[ARMeshAnchor class]] && !self.usePlanes) {
                ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
                
                NSUInteger index = [self.meshAnchors indexOfObjectPassingTest:^BOOL(ARMeshAnchor* obj, NSUInteger idx, BOOL* stop) {
                    return [obj.identifier isEqual:meshAnchor.identifier];
                }];
                
                if (index != NSNotFound) {
                    self.meshAnchors[index] = meshAnchor;
                }
            }
            else if ([anchor isKindOfClass:[ARPlaneAnchor class]]) {
                ARPlaneAnchor* planeAnchor = (ARPlaneAnchor*)anchor;
                
                NSUInteger index = [self.planeAnchors indexOfObjectPassingTest:^BOOL(ARPlaneAnchor* obj, NSUInteger idx, BOOL* stop) {
                    return [obj.identifier isEqual:planeAnchor.identifier];
                }];
                
                if (index != NSNotFound) {
                    self.planeAnchors[index] = planeAnchor;
                    
                    if (self.Owner) {
                        UE_LOG(LogTemp, VeryVerbose, TEXT("ARKit: Plane updated - Size: %.2f x %.2f"),
                            planeAnchor.extent.x,
                            planeAnchor.extent.z);
                    }
                }
            }
        }
    }
}

- (void)session:(ARSession *)session didRemoveAnchors:(NSArray<ARAnchor*>*)anchors {
    @autoreleasepool {
        for (ARAnchor* anchor in anchors) {
            if ([anchor isKindOfClass:[ARMeshAnchor class]]) {
                ARMeshAnchor* meshAnchor = (ARMeshAnchor*)anchor;
                [self.meshAnchors removeObject:meshAnchor];
            }
            else if ([anchor isKindOfClass:[ARPlaneAnchor class]]) {
                ARPlaneAnchor* planeAnchor = (ARPlaneAnchor*)anchor;
                [self.planeAnchors removeObject:planeAnchor];
            }
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
    MeshUpdateInterval = 1.0f;
    
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
        
        // Check for scene reconstruction support
        if ([ARWorldTrackingConfiguration supportsSceneReconstruction:ARSceneReconstructionMesh]) {
            delegate.usePlanes = NO;
            UE_LOG(LogTemp, Warning, TEXT("✅ Scene reconstruction supported (LiDAR detected) - Using mesh capture"));
        } else {
            delegate.usePlanes = YES;
            UE_LOG(LogTemp, Warning, TEXT("⚠️ Scene reconstruction not supported (no LiDAR) - Using plane detection"));
        }
        
        [(id)ARSession setDelegate:delegate];
        ARDelegate = (void*)delegate;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ ARKit initialized"));
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
        @autoreleasepool {
            [(id)ARSession pause];
            ARSession = nullptr;
        }
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
        TimeSinceLastUpdate += DeltaTime;
        
        if (TimeSinceLastUpdate >= 0.1f)
        {
            UpdateCameraFrame();
        }
        
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
    
    @autoreleasepool {
        ARWorldTrackingConfiguration* config = [[ARWorldTrackingConfiguration alloc] init];
        
        ARKitDelegate* delegate = (ARKitDelegate*)ARDelegate;
        
        if (!delegate.usePlanes && [ARWorldTrackingConfiguration supportsSceneReconstruction:ARSceneReconstructionMesh]) {
            config.sceneReconstruction = ARSceneReconstructionMesh;
            UE_LOG(LogTemp, Warning, TEXT("Scene reconstruction enabled (mesh mode)"));
        } else {
            // Use plane detection for non-LiDAR devices
            config.planeDetection = ARPlaneDetectionHorizontal | ARPlaneDetectionVertical;
            UE_LOG(LogTemp, Warning, TEXT("Plane detection enabled (non-LiDAR mode)"));
        }
        
        config.environmentTexturing = AREnvironmentTexturingAutomatic;
        config.lightEstimationEnabled = true;
        
        [(id)ARSession runWithConfiguration:config];
        bIsScanning = true;
    }
    
    if (CameraPreviewPlate)
    {
        CameraPreviewPlate->SetVisibility(true);
        CameraPreviewPlate->SetHiddenInGame(false);
        UE_LOG(LogTemp, Warning, TEXT("Camera preview plate visibility set to true"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ ARKit scanning started successfully"));
#else
    UE_LOG(LogTemp, Warning, TEXT("StartScanning called but not on iOS"));
    bIsScanning = true;
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
    
    @autoreleasepool {
        [(id)ARSession pause];
        bIsScanning = false;
    }
    
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
    
    @autoreleasepool {
        id sessionObj = (id)ARSession;
        ARFrame* currentFrame = [sessionObj currentFrame];
        
        if (currentFrame && currentFrame.capturedImage)
        {
            CVPixelBufferRef pixelBuffer = currentFrame.capturedImage;
            
            CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
            
            size_t width = CVPixelBufferGetWidth(pixelBuffer);
            size_t height = CVPixelBufferGetHeight(pixelBuffer);
            
            if (width > 0 && height > 0)
            {
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
                                *DestPtr++ = luma;
                                *DestPtr++ = luma;
                                *DestPtr++ = luma;
                                *DestPtr++ = 255;
                            }
                        }
                        
                        Mip.BulkData.Unlock();
                        CameraTexture->UpdateResource();
                    }
                }
            }
            
            CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        }
        
        currentFrame = nil;
    }
#endif
}

void AARKitRoomScanner::UpdateMeshFromARKit()
{
#if PLATFORM_IOS
    if (!ARDelegate) return;
    
    @autoreleasepool {
        ARKitDelegate* delegate = (ARKitDelegate*)ARDelegate;
        
        CurrentMeshData.Vertices.Empty();
        CurrentMeshData.Triangles.Empty();
        CurrentMeshData.Normals.Empty();
        
        int32 vertexOffset = 0;
        
        // Check if we should use planes or meshes
        if (delegate.usePlanes)
        {
            // Process plane anchors for non-LiDAR devices
            int32 planeCount = delegate.planeAnchors.count;
            
            if (planeCount > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Processing %d plane anchors"), planeCount);
            }
            
            for (ARPlaneAnchor* planeAnchor in delegate.planeAnchors)
            {
                // Get plane properties
                simd_float3 center = planeAnchor.center;
                simd_float3 extent = planeAnchor.extent;
                simd_float4x4 transform = planeAnchor.transform;
                
                // Create a simple quad for each plane
                float halfWidth = extent.x / 2.0f;
                float halfHeight = extent.z / 2.0f;
                
                // Define 4 corners of the plane
                simd_float3 corners[4] = {
                    simd_make_float3(center.x - halfWidth, center.y, center.z - halfHeight),
                    simd_make_float3(center.x + halfWidth, center.y, center.z - halfHeight),
                    simd_make_float3(center.x + halfWidth, center.y, center.z + halfHeight),
                    simd_make_float3(center.x - halfWidth, center.y, center.z + halfHeight)
                };
                
                // Transform and add vertices
                for (int i = 0; i < 4; i++)
                {
                    simd_float4 vertex4 = simd_make_float4(corners[i].x, corners[i].y, corners[i].z, 1.0f);
                    simd_float4 transformedVertex = simd_mul(transform, vertex4);
                    
                    FVector unrealVertex(
                        transformedVertex.x * 100.0f,
                        transformedVertex.z * 100.0f,
                        transformedVertex.y * 100.0f
                    );
                    
                    CurrentMeshData.Vertices.Add(unrealVertex);
                    
                    // Add normal (pointing up for horizontal planes, forward for vertical)
                    FVector normal = (planeAnchor.alignment == ARPlaneAnchorAlignmentHorizontal)
                        ? FVector(0, 0, 1)
                        : FVector(1, 0, 0);
                    CurrentMeshData.Normals.Add(normal);
                }
                
                // Add two triangles to form a quad
                CurrentMeshData.Triangles.Add(vertexOffset + 0);
                CurrentMeshData.Triangles.Add(vertexOffset + 1);
                CurrentMeshData.Triangles.Add(vertexOffset + 2);
                
                CurrentMeshData.Triangles.Add(vertexOffset + 0);
                CurrentMeshData.Triangles.Add(vertexOffset + 2);
                CurrentMeshData.Triangles.Add(vertexOffset + 3);
                
                vertexOffset += 4;
            }
        }
        else
        {
            // Process mesh anchors for LiDAR devices (original code)
            int32 meshAnchorCount = delegate.meshAnchors.count;
            
            if (meshAnchorCount > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Processing %d mesh anchors"), meshAnchorCount);
            }
            
            for (ARMeshAnchor* meshAnchor in delegate.meshAnchors) {
                ARMeshGeometry* geometry = meshAnchor.geometry;
                
                if (!geometry) continue;
                
                ARGeometrySource* vertexSource = geometry.vertices;
                NSInteger vertexCount = vertexSource.count;
                id<MTLBuffer> vertexBuffer = vertexSource.buffer;
                const simd_float3* vertices = (const simd_float3*)[vertexBuffer contents];
                
                ARGeometryElement* faceSource = geometry.faces;
                NSInteger faceCount = faceSource.count;
                id<MTLBuffer> faceBuffer = faceSource.buffer;
                const void* faceBytes = [faceBuffer contents];
                
                const simd_float3* normals = nullptr;
                if (geometry.normals) {
                    ARGeometrySource* normalSource = geometry.normals;
                    id<MTLBuffer> normalBuffer = normalSource.buffer;
                    normals = (const simd_float3*)[normalBuffer contents];
                }
                
                simd_float4x4 transform = meshAnchor.transform;
                
                for (NSInteger i = 0; i < vertexCount; i++) {
                    simd_float3 vertex = vertices[i];
                    simd_float4 vertex4 = simd_make_float4(vertex.x, vertex.y, vertex.z, 1.0f);
                    simd_float4 transformedVertex = simd_mul(transform, vertex4);
                    
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
                
                if (faceSource.bytesPerIndex == 2) {
                    const uint16_t* indices = (const uint16_t*)faceBytes;
                    for (NSInteger i = 0; i < faceCount * 3; i += 3) {
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i]);
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 2]);
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 1]);
                    }
                } else if (faceSource.bytesPerIndex == 4) {
                    const uint32_t* indices = (const uint32_t*)faceBytes;
                    for (NSInteger i = 0; i < faceCount * 3; i += 3) {
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i]);
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 2]);
                        CurrentMeshData.Triangles.Add(vertexOffset + indices[i + 1]);
                    }
                }
                
                vertexOffset += vertexCount;
            }
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
            
            UE_LOG(LogTemp, Warning, TEXT("Mesh updated: %d vertices, %d triangles"),
                CurrentMeshData.VertexCount, CurrentMeshData.TriangleCount);
        }
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
        return;
    }
    
    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
    if (Widget)
    {
        Widget->AddToViewport();
        UE_LOG(LogTemp, Warning, TEXT("✅ HUD Widget created and added to viewport"));
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
