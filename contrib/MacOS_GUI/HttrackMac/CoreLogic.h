//
//  CoreLogic.h
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import <Foundation/Foundation.h>

#import "htscore.h"

#import "AppDelegate.h"
#import "Models/ModelsApp.h"

NS_ASSUME_NONNULL_BEGIN

@interface CoreLogic : NSObject {
    httrackp *_httrack_opt;
}
@property (assign) IBOutlet AppDelegate *app;
@property (readonly) MyDirectoryElements * websites;

-(void)dowloadSite:(NSString*) url;
-(void)indexOfDownloadedSites:(MyDirectoryElements *) arbo;
@end

NS_ASSUME_NONNULL_END
