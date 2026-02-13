
#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

// overrides
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;

// ours
-(void)changeWindowSubtitle:(NSString*)newSubtitle;
-(void)warnUser:(NSError*)error; /* show a popup warning */
@end

