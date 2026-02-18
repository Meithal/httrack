//#import <Availability.h>

#import "AppDelegate.h"

@interface AppDelegate ()

@property (strong) IBOutlet NSView *AppIcon;
@property (strong) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    
    NSLog(@"tile size= %@", NSStringFromSize( _window.dockTile.size));
    NSView* w = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 128, 128)];
    self.AppIcon.wantsLayer = YES;
    self.AppIcon.layer.backgroundColor = NSColor.greenColor.CGColor;
    self.window.dockTile.contentView = _AppIcon;
    [self.window.dockTile.contentView addSubview:w];
    self.window.dockTile.badgeLabel = @"toto";
    [self.window.dockTile setShowsApplicationBadge:YES];
    [self.window.dockTile display];
    //_window.toolbarStyle = NSWindowToolbarStyleUnified;//
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [_logic release];
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

-(void)changeWindowSubtitle:(NSString*)newSubtitle
{
    if(@available(macOS 11.0, *)) {
        self.window.subtitle = newSubtitle;
    }
}

-(CoreLogic*)getLogic {
    if(_logic == nil)
        _logic = [[CoreLogic alloc] init];
    return _logic;
}

-(void)warnUser:(NSError*)error
{
    [[NSAlert alertWithError:error] runModal];
}
@end
