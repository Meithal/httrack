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
@interface ControllerMainMenu : NSObject<CoreLogicDelegate>
{
    IBOutlet AppDelegate *_AppDelegate;
    CoreLogic* _logic;
    CoreLogicDelegate * _logicDelegate;
    IBOutlet NSButton* _downloadButton;
}

@property (assign) IBOutlet NSTextField *httrSiteUrl;

@end


NS_ASSUME_NONNULL_END
