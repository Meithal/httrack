#import "ViewMain.h"
#import "Controllers/ControllerMainMenu.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainSplitViewDelegate : NSObject<NSSplitViewDelegate>
{}
@end

@implementation MainSplitViewDelegate
- (void) splitView:(NSSplitView *) splitView
resizeSubviewsWithOldSize:(NSSize) oldSize
{
//    NSLog(@"oldsize %@\n", NSStringFromSize(oldSize));
    NSView* left = splitView.subviews[0];
    NSView* right = splitView.subviews[1];
    //splitView.
    
    CGFloat fixedWidth = left.frame.size.width;
    CGFloat dividerWidth = splitView.dividerThickness;
    
    left.frame = NSMakeRect(0, 0, fixedWidth, splitView.frame.size.height);
    
    CGFloat remainder = splitView.frame.size.width - fixedWidth - dividerWidth;
    right.frame = NSMakeRect(fixedWidth + dividerWidth, 0, remainder, splitView.frame.size.height);
}

- (CGFloat) splitView:(NSSplitView *) splitView
constrainMinCoordinate:(CGFloat) proposedMinimumPosition
          ofSubviewAt:(NSInteger) dividerIndex
{
    
    return 200;
}
- (CGFloat) splitView:(NSSplitView *) splitView
constrainMaxCoordinate:(CGFloat) proposedMinimumPosition
          ofSubviewAt:(NSInteger) dividerIndex
{
    return splitView.frame.size.width - 200;
}
//- (instancetype)init
//{
//    self = [super init];
//    if (self) {
//        ;
//    }
//    
//    return self;
//}

//- (void)awakeFromNib
//{
//    [super awakeFromNib];
//    
//    NSSplitViewController *svc = [[NSSplitViewController alloc] init];
//
//    NSViewController *leftVC = [[NSViewController alloc] init];
//    //leftVC.view = self.left[0];   // outlet from XIB
//
//    NSViewController *rightVC = [[NSViewController alloc] init];
//    //rightVC.view = self.splitView.subviews[1]; // outlet from XIB
//
//    [svc addSplitViewItem:
//        [NSSplitViewItem splitViewItemWithViewController:leftVC]];
//    [svc addSplitViewItem:
//        [NSSplitViewItem splitViewItemWithViewController:rightVC]];
//
//    svc.view.frame = self.view.bounds;
//    svc.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
//
//    [self addChildViewController:svc];
//    [self.view addSubview:svc.view];
//    return;
//}
//- (void)viewDidLoad
//{
//    [super viewDidLoad];
//
//    NSSplitViewController *svc = [[NSSplitViewController alloc] init];
//
//    NSViewController *leftVC = [[NSViewController alloc] init];
//    leftVC.view = self.splitView.subviews[0];   // outlet from XIB
//
//    NSViewController *rightVC = [[NSViewController alloc] init];
//    rightVC.view = self.splitView.subviews[1]; // outlet from XIB
//
//    [svc addSplitViewItem:
//        [NSSplitViewItem splitViewItemWithViewController:leftVC]];
//    [svc addSplitViewItem:
//        [NSSplitViewItem splitViewItemWithViewController:rightVC]];
//
//    svc.view.frame = self.view.bounds;
//    svc.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
//
//    [self addChildViewController:svc];
//    [self.view addSubview:svc.view];
//}
@end

@interface ProjectsDataSource: NSObject<NSOutlineViewDataSource, NSOutlineViewDelegate>
- (NSInteger) numberOfRowsInTableView:(NSTableView *) tableView;
- (id) tableView:(NSTableView *) tableView
objectValueForTableColumn:(NSTableColumn *) tableColumn
             row:(NSInteger) row;
@end

@implementation ProjectsDataSource
- (NSInteger) numberOfRowsInTableView:(NSTableView *) tableView
{
    return 4;
}
- (id) tableView:(NSTableView *) tableView
objectValueForTableColumn:(NSTableColumn *) tableColumn
             row:(NSInteger) row;
{
    return @"toto";
}
@end


@implementation ViewMain
- (IBAction)httrDowloadButton:(id)sender {
    NSLog(@"Push %@", [self.httrSiteUrl stringValue]);
    [self.controller DownloadSite:[self.httrSiteUrl stringValue]];
    
}


@end

NS_ASSUME_NONNULL_END
