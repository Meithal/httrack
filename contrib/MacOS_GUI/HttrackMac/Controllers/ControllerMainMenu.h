#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

enum {
    HTR_CONTROL_PLAY = 0,
    HTR_CONTROL_PAUSE,
    HTR_CONTROL_STOP,
};
@class ProjectsOutlineView;
@class MonContenuPreview;
/**
 Notre "controlleur" qui fait l'interface entre l'UI et la logique+données
 */
@interface ControllerMainMenu : NSViewController<CoreLogicDelegate>
{
    IBOutlet AppDelegate *_AppDelegate;
    CoreLogic* _logic;
    CoreLogicDelegate * _logicDelegate;
    
    IBOutlet ProjectsOutlineView * _projectsOutlineView;
    IBOutlet NSButton* _downloadButton;
    IBOutlet NSSegmentedControl* _playpausestopControl;
    
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
    
    IBOutlet MonContenuPreview* _contenuPreview;
}

@property (assign) IBOutlet NSTextField *httrSiteUrl;

-(void)updateState:(hts_stat_struct *) stats;
-(ProjectsOutlineView*)projectsOutlineView;
-(MonContenuPreview*) contenuPreview;

@end


@interface ProjectsDataSource: NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate>
{
    IBOutlet AppDelegate *_delegate;
    CoreLogic* _logic;
}
@end

@class ControllerMainMenu;

@interface ProjectsOutlineView: NSOutlineView
{
    IBOutlet ControllerMainMenu* _mainController;
}
-(ControllerMainMenu*)mainController;
@end

@interface MonContenuPreview: NSView<WKUIDelegate> {
}
-(void)mainChangePreview:(NSString*)chemin;
@end


NS_ASSUME_NONNULL_END
