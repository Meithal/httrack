#import "ViewMain.h"
#import "Controllers/ControllerMainMenu.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainSplitViewDelegate : NSObject<NSSplitViewDelegate>
{}
@end

#pragma mark main split view delagate

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

#pragma mark NSOutlineViewDataSource delegates
- (BOOL)outlineView:(nonnull NSOutlineView *)outlineView isItemExpandable:(nonnull MyDirectoryElements *)item {
    if(item.class == MyDowloadableFile.class)
        return NO;
    return item.directories.count || item.files.count;
}

- (NSInteger)outlineView:(nonnull NSOutlineView *)outlineView numberOfChildrenOfItem:(nullable MyDirectoryElements *)item {
    if(item == nil)
        return self.telechargements.websites.directories.count;
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
        MyDowloadableFile * item_cast = item;
        if([tableColumn.identifier isEqual:@"PageName"])
            return item_cast.name;
        else if(([tableColumn.identifier isEqual:@"Avancement"]))
            return item_cast.downloadAdvancement;
        else raise(42);

    }
}

- (nonnull id)outlineView:(nonnull NSOutlineView *)outlineView child:(NSInteger)index ofItem:(nullable MyDirectoryElements*)item {
    if(item == nil)
        return self.telechargements.websites.directories[0];
    else {
        if(index < item.directories.count)
            return item.directories[index];
        else
            return item.files[index - item.directories.count];
    }
}

#pragma mark NSOutlineViewDelegate overrides
- (BOOL) outlineView:(NSOutlineView *) outlineView
         isGroupItem:(MyDirectoryElements *) item
{
    if(item.class != MyDirectoryElements.class)
        return NO;
    return item.depth < 2;
}
@end

@implementation ViewMain
- (IBAction)httrDowloadButton:(id)sender {
    NSLog(@"Push %@", [self.httrSiteUrl stringValue]);
    [self.controller DownloadSite:[self.httrSiteUrl stringValue]];
    
}


@end

NS_ASSUME_NONNULL_END
