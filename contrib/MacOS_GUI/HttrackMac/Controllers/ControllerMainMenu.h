//
//  ControllerMainMenu.h
//  HttrackMac
//
//  Created by ivo on 06/02/2026.
//

#import <Foundation/Foundation.h>

#import "../AppDelegate.h"


NS_ASSUME_NONNULL_BEGIN

@interface ControllerMainMenu : NSObject
@property (assign) IBOutlet AppDelegate *app;

-(void)DownloadSite:(NSString*) url;
@end

NS_ASSUME_NONNULL_END
