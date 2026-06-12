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
    id _objCallback;
    SEL _loopCallback;
    enum CoreLogicState {CORELOGIC_STATE_STOPPED,CORELOGIC_STATE_RUNNING, CORELOGIC_STATE_PAUSED} _state;
    MyDirectoryElements * _websites;
}
-(MyDirectoryElements *) websites;
-(void)dowloadSite:(NSString*) url onError:(void (^)(NSString *, NSErrorDomain, NSInteger)) onError;
-(void)indexOfDownloadedSites:(MyDirectoryElements *) arbo;

-(void)setDelegate:(nullable id)newDelegate;
-(CoreLogicDelegate*)delegate;

-(void)setLoopCallback:(nullable SEL) callback withObject: (nullable id) obj;
-(SEL)loopCallback;
-(id)objCallback;
-(nullable httrackp*)httrack_opt;

-(void)pauseMirror:(int)p;
-(void)stopMirror;
-(enum CoreLogicState) state;
-(void)setState:(enum CoreLogicState)state;
@end

@protocol CoreLogicDelegate<NSObject>
-(BOOL)coreLogicDownloadWillStart:(CoreLogic*)sender;
-(void)coreLogicDownloadDidStop:(CoreLogic*)sender;
-(void)coreLogicDownloadDidPause:(CoreLogic*)sender;
-(void)coreLogicPageAdded:(CoreLogic*)sender;
-(void)coreLogicDownloadDidAdvance:(CoreLogic*)sender path:(NSString*) path domain:(NSString*) domain ratio:(float)ratio;
@end

@interface HtmrEventDispatcher: NSObject {
    NSMutableArray<void(^)(void)> * listeners;
}
-(void)addEventListener:(void(^)(void)) fun;
-(BOOL)removeEventListener:(void(^)(void)) fun;

@end

NS_ASSUME_NONNULL_END
