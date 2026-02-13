//
//  ControllerMainMenu.h
//  HttrackMac
//
//  Created by ivo on 06/02/2026.
//

#import <Foundation/Foundation.h>

#import "../AppDelegate.h"
#import "../Models/ModelsApp.h"
#import "../CoreLogic.h"

NS_ASSUME_NONNULL_BEGIN

@interface ControllerMainMenu : NSTreeController
@property (assign) IBOutlet AppDelegate *app;
@property (assign) IBOutlet ModelsApp * models;
@property (assign) IBOutlet CoreLogic *coreLogic;

-(void)DownloadSite:(NSString*) url;
@end

NS_ASSUME_NONNULL_END
