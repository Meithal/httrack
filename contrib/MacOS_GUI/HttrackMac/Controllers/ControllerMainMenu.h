#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface ProjectsDataSource: NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate>
{
    IBOutlet AppDelegate *_delegate;
    CoreLogic* _logic;
}
@end

@interface ProjectsOutlineView: NSOutlineView
@end


/**
 Notre "controlleur" qui fait l'interface entre l'UI et la logique+données
 */
@interface ControllerMainMenu : NSObject<CoreLogicDelegate>
{
    IBOutlet AppDelegate *_AppDelegate;
    CoreLogic* _logic;
    CoreLogicDelegate * _logicDelegate;
    IBOutlet NSButton* _downloadButton;
    IBOutlet NSButton* _pauseButton;
    
    IBOutlet NSTextField* _httrTotalRecvLabel;
    IBOutlet NSTextField* _httrTotalBytesWrittenLabel;
    IBOutlet NSTextField* _httrTimeStartLabel;
    IBOutlet NSTextField* _httrTotalUnpackedReceivedLabel;
    IBOutlet NSTextField* _httrTotalPackedReceivedLabel;
    IBOutlet NSTextField* _httrTotalPackedFilesLabel;
    IBOutlet NSTextField* _httrTotalWrittenFilesLabel;
    IBOutlet NSTextField* _httrTotalUpdatedFilesLabel;
    IBOutlet NSTextField* _httrTotalBackgroundFilesLabel;
    IBOutlet NSTextField* _httrTotalSockRequestsLabel;
    IBOutlet NSTextField* _httrTotalSocksAllocatedLabel;
    IBOutlet NSTextField* _httrTotalSocksLabel;
    IBOutlet NSTextField* _httrTotalErrorsLabel;
    IBOutlet NSTextField* _httrTotalFrontErrorsLabel;
    IBOutlet NSTextField* _httrTotalWarningsLabel;
    IBOutlet NSTextField* _httrTotalInfosLabel;
    IBOutlet NSTextField* _httrTotalBackgroundAnticLabel;
    IBOutlet NSTextField* _httrTotalTransferedLabel;
    IBOutlet NSTextField* _httrRateLabel;
    IBOutlet NSTextField* _httrLastConnectLabel;
    IBOutlet NSTextField* _httrLastRequestLabel;
}

@property (assign) IBOutlet NSTextField *httrSiteUrl;

-(void)updateStats:(hts_stat_struct *) stats;

@end


NS_ASSUME_NONNULL_END
