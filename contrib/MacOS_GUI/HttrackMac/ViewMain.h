#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "Controllers/ControllerMainMenu.h"

NS_ASSUME_NONNULL_BEGIN


@interface ViewMain : NSObject
@property (assign) IBOutlet NSTextField *httrSiteUrl;
@property (assign) IBOutlet ControllerMainMenu *controller;

@end

NS_ASSUME_NONNULL_END
