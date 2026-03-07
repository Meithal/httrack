#import "../AppDelegate.h"

#import "ControllerMainMenu.h"

NS_ASSUME_NONNULL_BEGIN

#pragma mark Notre barre de recherche
@interface MySearchInputField: NSSearchField
@property (copy) NSArray<NSString *> * placeholderStrings;
@end
@implementation MySearchInputField
@dynamic placeholderStrings;

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.placeholderStrings = @[
           @"toto",
           @"tata",
           @"titi"
        ];
    }
    return self;
}

-(void)awakeFromNib
{
    NSSearchFieldCell * cell = [self cell];
    if(@available(macOS 11.0, *)) {
        [cell.searchButtonCell setImage: [NSImage imageWithSystemSymbolName:@"tray.and.arrow.down" accessibilityDescription:@"The URL to download"]];
    }
}
@end

@interface MainSplitViewDelegate : NSObject<NSSplitViewDelegate>
{}
@end

#pragma mark MainSplitViewDelegate
@implementation MainSplitViewDelegate
- (void) splitView:(NSSplitView *) splitView
resizeSubviewsWithOldSize:(NSSize) oldSize
{
    NSView* left = splitView.subviews[0];
    NSView* right = splitView.subviews[1];
    
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
@end

@interface VerticalTextCell: NSTextFieldCell
@end

@implementation VerticalTextCell
- (NSRect)drawingRectForBounds:(NSRect)rect
{
    NSFont * font = self.font;
    return NSRectFromCGRect(CGRectMake(rect.origin.x, rect.origin.y + font.xHeight / 2, rect.size.width, rect.size.height)); // centre verticalement le texte
}
@end

@implementation ProjectsDataSource
-(void)awakeFromNib {
    _logic = _delegate.getLogic;
}
#pragma mark NSOutlineViewDataSource
- (BOOL)outlineView:(nonnull NSOutlineView *)outlineView isItemExpandable:(nonnull MyDirectoryElements *)item {
    if(item.class == MyDowloadableFile.class)
        return NO;
    NSLevelIndicatorCell * c;
    return item.directories.count || item.files.count;
}

- (NSInteger)outlineView:(nonnull NSOutlineView *)outlineView numberOfChildrenOfItem:(nullable MyDirectoryElements *)item {
    if(!_logic)
        return 0;
    if(item == nil)
        return _logic.websites.directories.count;
    else if (item.class == MyDowloadableFile.class)
        return 0;
    else
        return item.directories.count + item.files.count;
}

- (nullable id)outlineView:(nonnull NSOutlineView *)outlineView objectValueForTableColumn:(nullable NSTableColumn *)tableColumn byItem:(nullable MyDirectoryElements *)item {
    if(item == nil) return @"nil";
    
    if(item.class == MyDirectoryElements.class) {
        if([tableColumn.identifier isEqual:@"PageName"])
            return item.name;
        else if(([tableColumn.identifier isEqual:@"Avancement"]))
            return nil;
        else raise(42);
    } else if (item.class == MyDowloadableFile.class) {
        MyDowloadableFile * item_cast = (MyDowloadableFile*)item;
        if([tableColumn.identifier isEqual:@"PageName"])
            return item_cast.name;
        else if(([tableColumn.identifier isEqual:@"Avancement"]))
            return item_cast.downloadAdvancement;
        else raise(42);
    }
}

- (nonnull id)outlineView:(nonnull NSOutlineView *)outlineView child:(NSInteger)index ofItem:(nullable MyDirectoryElements*)item {
    if(item == nil)
        return _logic.websites.directories[0];
    else {
        if(index < item.directories.count)
            return item.directories[index];
        else
            return item.files[index - item.directories.count];
    }
}

#pragma mark NSOutlineViewDelegate
- (BOOL) outlineView:(NSOutlineView *) outlineView
         isGroupItem:(MyDirectoryElements *) item
{
    if(item.class != MyDirectoryElements.class)
        return NO;
    return item.depth < 2;
}
@end

#pragma mark ProjectsOutlineView

@implementation ProjectsOutlineView
-(void)awakeFromNib{
    [self reloadData];
}
@end

#pragma mark ControllerMainMenu
@implementation ControllerMainMenu
-(void) awakeFromNib {
    
    _logic = _AppDelegate.getLogic;
    [_logic setDelegate:self];
}

- (IBAction)httrDowloadButton:(NSButton *)sender {
    NSLog(@"button click %@\n", sender);
    NSLog(@"Push %@", [self.httrSiteUrl stringValue]);
    
    [_AppDelegate changeWindowSubtitle:[self.httrSiteUrl stringValue]];
    //[self.coreLogic indexOfDownloadedSites];
    [_logic
     dowloadSite:[self.httrSiteUrl stringValue]
     onError:^(NSString *description, NSErrorDomain domain, NSInteger code) {
        [_AppDelegate warnUser:description domain:domain code:code];
    }];
    
    [sender setEnabled:false];
}
-(BOOL)coreLogicDownloadWillStart:(CoreLogicDelegate *)sender {
    NSLog(@"Download did start");
    
    [_downloadButton setEnabled:NO];
    return YES;
}

-(void)coreLogicDownloadDidStop:(CoreLogic*)sender {
    [_downloadButton setEnabled:YES];
}


@end

NS_ASSUME_NONNULL_END
