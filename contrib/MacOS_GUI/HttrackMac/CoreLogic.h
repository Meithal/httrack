//
//  CoreLogic.h
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import <Foundation/Foundation.h>

#import "htscore.h"

#import "AppDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@interface CoreLogic : NSObject {
    httrackp *_httrack_opt;
}
@property (assign) IBOutlet AppDelegate *app;

-(void)dowloadSite:(NSString*) url;
-(NSArray *)indexOfDownloadedSites;
@end

NS_ASSUME_NONNULL_END
