#include "IOSShareBridge.h"
#include "ISLog.h"
#if PLATFORM_IOS
#import <UIKit/UIKit.h>
static UIViewController* TopVC(){ UIViewController* root=[UIApplication sharedApplication].keyWindow.rootViewController; while(root.presentedViewController) root=root.presentedViewController; return root; }
void IOS_ShowShareSheetForFile(const TCHAR* InPath){
    NSString* nsPath=[NSString stringWithUTF8String:TCHAR_TO_UTF8(InPath)];
    if(![[NSFileManager defaultManager] fileExistsAtPath:nsPath]) { UE_LOG(LogIntelliSpace, Warning, TEXT("[Share] Missing file: %s"), *FString(nsPath)); return; }
    NSURL* url=[NSURL fileURLWithPath:nsPath];
    UIActivityViewController* act=[[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
    act.modalPresentationStyle=UIModalPresentationPopover; [TopVC() presentViewController:act animated:YES completion:nil]; [act release];
}
#endif
