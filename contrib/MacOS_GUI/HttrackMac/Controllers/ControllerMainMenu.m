//
//  ControllerMainMenu.m
//  HttrackMac
//
//  Created by ivo on 06/02/2026.
//

#import "ControllerMainMenu.h"

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN





@implementation ControllerMainMenu

-(void)DownloadSite:(NSString*) url {
    NSLog(@"Controller %@", url);
    
    [self.app changeWindowSubtitle:url];
    //[self.coreLogic indexOfDownloadedSites];
    [self.coreLogic dowloadSite:url];
}
@end

NS_ASSUME_NONNULL_END
