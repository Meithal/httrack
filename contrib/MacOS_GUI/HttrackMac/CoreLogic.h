//
//  CoreLogic.h
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import <Foundation/Foundation.h>

#import "htscore.h"

#import "Models/ModelsApp.h"

NS_ASSUME_NONNULL_BEGIN

@class HtmrEventDispatcher;
@class CoreLogicDelegate;

@interface CoreLogic : NSObject {
    httrackp *_httrack_opt;
    HtmrEventDispatcher * _eventDispatcher;
    CoreLogicDelegate * _delegate;
}
@property (readonly) MyDirectoryElements * websites;

-(void)dowloadSite:(NSString*) url onError:(void (^)(NSString *, NSErrorDomain, NSInteger)) onError;
-(void)indexOfDownloadedSites:(MyDirectoryElements *) arbo;

-(void)setDelegate:(id)newDelegate;
-(CoreLogicDelegate*):delegate;
@end


@protocol CoreLogicDelegate<NSObject>
-(BOOL)coreLogicDownloadWillStart:(CoreLogic*)sender;
-(void)coreLogicDownloadDidStop:(CoreLogic*)sender;
@end


@interface HtmrEventDispatcher: NSObject {
    NSMutableArray<void(^)(void)> * listeners;
}
-(void)addEventListener:(void(^)(void)) fun;
-(BOOL)removeEventListener:(void(^)(void)) fun;

@end

NS_ASSUME_NONNULL_END
