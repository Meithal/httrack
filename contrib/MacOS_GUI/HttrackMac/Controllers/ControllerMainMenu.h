#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "ControllerMainMenu.h"

NS_ASSUME_NONNULL_BEGIN


@interface ProjectsDataSource: NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate>
@property (assign) IBOutlet CoreLogic *telechargements;
@end

@interface ControllerMainMenu : NSObject
@property (assign) IBOutlet NSTextField *httrSiteUrl;

@end

NS_ASSUME_NONNULL_END
