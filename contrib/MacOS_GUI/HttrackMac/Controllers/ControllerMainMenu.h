#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface ProjectsDataSource: NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate>
{
    IBOutlet AppDelegate *_delegate;
    CoreLogic* _logic;
}
@end

@interface ProjectsOutlineView: NSOutlineView
@end

@interface ControllerMainMenu : NSObject
{
    IBOutlet AppDelegate *_delegate;
    CoreLogic* _logic;
}
@property (assign) IBOutlet NSTextField *httrSiteUrl;

@end

NS_ASSUME_NONNULL_END
