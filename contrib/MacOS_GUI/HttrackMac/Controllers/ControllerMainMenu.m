//
//  ControllerMainMenu.m
//  HttrackMac
//
//  Created by ivo on 06/02/2026.
//

#import "ControllerMainMenu.h"
#import "../CoreLogic.h"

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN


@implementation ControllerMainMenu

-(void)DownloadSite:(NSString*) url {
    NSLog(@"Controller %@", url);
    
    [(AppDelegate*)([NSApplication sharedApplication].delegate) changeWindowSubtitle:url];
    //[self.coreLogic indexOfDownloadedSites];
    [[(AppDelegate*)([NSApplication sharedApplication].delegate) getLogic] dowloadSite:url onError:^(NSDictionary *errorDictionary, NSErrorDomain domain) {
        [(AppDelegate*)([NSApplication sharedApplication].delegate) warnUser:[[NSError alloc] initWithDomain:domain code:NSURLErrorBadURL userInfo:errorDictionary]];
    }];
    
    

}
@end

NS_ASSUME_NONNULL_END
