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


/**
 Notre "controlleur" qui fait l'interface entre l'UI et la logique+données
 */
@interface ControllerMainMenu : NSObject
{
    IBOutlet AppDelegate *_delegate;
    CoreLogic* _logic;
}

@property (assign) IBOutlet NSTextField *httrSiteUrl;

@end


NS_ASSUME_NONNULL_END
