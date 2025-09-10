// ScannerHUD.cpp
#include "ScannerHUD.h"
#include "ARKitRoomScanner.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

bool UScannerHUD::Initialize()
{
    bool Success = Super::Initialize();
    if (!Success) return false;
    
    CreateUI();
    return true;
}

void UScannerHUD::NativeConstruct()
{
    Super::NativeConstruct();
}

void UScannerHUD::CreateUI()
{
    // Create the UI programmatically
    UCanvasPanel* RootPanel = NewObject<UCanvasPanel>(this);
    
    // Create vertical box for buttons
    UVerticalBox* ButtonBox = NewObject<UVerticalBox>(this);
    
    // Create buttons
    StartButton = NewObject<UButton>(ButtonBox);
    StopButton = NewObject<UButton>(ButtonBox);
    ClearButton = NewObject<UButton>(ButtonBox);
    ExportButton = NewObject<UButton>(ButtonBox);
    
    // Create text labels for buttons
    UTextBlock* StartText = NewObject<UTextBlock>(this);
    StartText->SetText(FText::FromString(TEXT("Start Scan")));
    StartButton->AddChild(StartText);
    
    UTextBlock* StopText = NewObject<UTextBlock>(this);
    StopText->SetText(FText::FromString(TEXT("Stop Scan")));
    StopButton->AddChild(StopText);
    
    UTextBlock* ClearText = NewObject<UTextBlock>(this);
    ClearText->SetText(FText::FromString(TEXT("Clear Mesh")));
    ClearButton->AddChild(ClearText);
    
    UTextBlock* ExportText = NewObject<UTextBlock>(this);
    ExportText->SetText(FText::FromString(TEXT("Export Mesh")));
    ExportButton->AddChild(ExportText);
    
    // Status text
    StatusText = NewObject<UTextBlock>(this);
    StatusText->SetText(FText::FromString(TEXT("Ready")));
    
    // Add buttons to vertical box
    ButtonBox->AddChild(StartButton);
    ButtonBox->AddChild(StopButton);
    ButtonBox->AddChild(ClearButton);
    ButtonBox->AddChild(ExportButton);
    ButtonBox->AddChild(StatusText);
    
    // Add to root
    RootPanel->AddChild(ButtonBox);
    
    // Position the panel
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ButtonBox->Slot))
    {
        CanvasSlot->SetPosition(FVector2D(50, 50));
        CanvasSlot->SetSize(FVector2D(200, 300));
    }
    
    // Bind button clicks
    if (StartButton)
        StartButton->OnClicked.AddDynamic(this, &UScannerHUD::OnStartButtonClicked);
    
    if (StopButton)
        StopButton->OnClicked.AddDynamic(this, &UScannerHUD::OnStopButtonClicked);
    
    if (ClearButton)
        ClearButton->OnClicked.AddDynamic(this, &UScannerHUD::OnClearButtonClicked);
    
    if (ExportButton)
        ExportButton->OnClicked.AddDynamic(this, &UScannerHUD::OnExportButtonClicked);
    
    // Initially disable stop button
    if (StopButton) StopButton->SetIsEnabled(false);
}

void UScannerHUD::OnStartButtonClicked()
{
    if (ScannerActor)
    {
        ScannerActor->StartScanning();
        
        if (StartButton) StartButton->SetIsEnabled(false);
        if (StopButton) StopButton->SetIsEnabled(true);
    }
}

void UScannerHUD::OnStopButtonClicked()
{
    if (ScannerActor)
    {
        ScannerActor->StopScanning();
        
        if (StartButton) StartButton->SetIsEnabled(true);
        if (StopButton) StopButton->SetIsEnabled(false);
    }
}

void UScannerHUD::OnClearButtonClicked()
{
    if (ScannerActor)
    {
        ScannerActor->ClearMesh();
    }
}

void UScannerHUD::OnExportButtonClicked()
{
    if (ScannerActor)
    {
        FString FileName = FString::Printf(TEXT("RoomScan_%lld"), FDateTime::Now().GetTicks());
        ScannerActor->ExportMesh(FileName);
    }
}

void UScannerHUD::SetScannerActor(AARKitRoomScanner* Scanner)
{
    ScannerActor = Scanner;
}

void UScannerHUD::UpdateStatus(const FString& Status)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Status));
    }
}
