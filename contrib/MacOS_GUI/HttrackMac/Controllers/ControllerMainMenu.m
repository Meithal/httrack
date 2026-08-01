#import "../AppDelegate.h"

#import "ControllerMainMenu.h"

#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark ControllerMainMenu
@implementation ControllerMainMenu
-(void) awakeFromNib {
    
    _logic = _AppDelegate.getLogic;
    [_logic setDelegate:self];
    [_logic setLoopCallback:@selector(updateState:) withObject:self];
}

-(void) dealloc {
    [_logic setDelegate:nil];
    [_logic setLoopCallback:nil withObject:nil];
    [super dealloc];
}

-(ProjectsOutlineView*)projectsOutlineView {
    return _projectsOutlineView;
}

-(MonContenuPreview*) contenuPreview {
    return _contenuPreview;
}


- (IBAction)httrDowloadButton:(NSButton *)sender {
    NSLog(@"Push %@", [self.httrSiteUrl stringValue]);
    
    [_AppDelegate changeWindowSubtitle:[self.httrSiteUrl stringValue]];
    //[self.coreLogic indexOfDownloadedSites];
    [_logic
     dowloadSite:[self.httrSiteUrl stringValue]
     onError:^(NSString *description, NSErrorDomain domain, NSInteger code) {
        [_AppDelegate warnUser:description domain:domain code:code];
    }];
}

-(IBAction)segmentedControl:(NSSegmentedControl*)sender {
    //NSLog(@"Segment %@\n", [sender selectedSegment]);
    switch ([sender selectedTag]) {
        case HTR_CONTROL_PLAY:
            [_logic pauseMirror:0];
            break;
        case HTR_CONTROL_PAUSE:
            [_logic pauseMirror:1];
            break;
        case HTR_CONTROL_STOP:
            [_logic stopMirror];
            break;
        default:
            break;
    }
}

#pragma mark mise a jour de l'appli en fonction des infos de httrack
-(void)updateState:(hts_stat_struct *) stats {
    if(stats == NULL)
        return;
    
    [_httrTotalRecvLabel setStringValue:[NSString stringWithFormat:@"%ld bytes", stats->HTS_TOTAL_RECV]];
    [_httrTotalBytesWrittenLabel setStringValue:[NSString stringWithFormat:@"%ld bytes", stats->stat_bytes]];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateStyle:NSDateFormatterMediumStyle];
    [formatter setTimeStyle:NSDateFormatterMediumStyle];
    [formatter setLocale:[NSLocale currentLocale]];

    NSString *localized = [formatter stringFromDate:[NSDate dateWithTimeIntervalSince1970: stats->stat_timestart]];
    [_httrTimeStartLabel setStringValue:localized];
    
    [_httrTotalUnpackedReceivedLabel setStringValue:[NSString stringWithFormat:@"%ld", stats->total_unpacked]];
    [_httrTotalPackedReceivedLabel setStringValue:[NSString stringWithFormat:@"%ld", stats->total_packed]];
    [_httrTotalPackedFilesLabel setStringValue:[NSString stringWithFormat:@"%d", stats->total_packedfiles]];
    [_httrTotalWrittenFilesLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_files]];
    [_httrTotalUpdatedFilesLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_updated_files]];
    [_httrTotalBackgroundFilesLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_background]];
    [_httrTotalSockRequestsLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_nrequests]];
    [_httrTotalSocksAllocatedLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_sockid]];
    [_httrTotalSocksLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_nsocket]];
    [_httrTotalErrorsLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_errors]];
    [_httrTotalFrontErrorsLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_errors_front]];
    [_httrTotalWarningsLabel setStringValue:[NSString stringWithFormat:@"%d", stats->stat_warnings]];
    [_httrTotalInfosLabel setStringValue:[NSString stringWithFormat:@"%d bytes", stats->stat_infos]];
    [_httrTotalBackgroundAnticLabel setStringValue:[NSString stringWithFormat:@"%d", stats->nbk]];
    [_httrTotalTransferedLabel setStringValue:[NSString stringWithFormat:@"%ld bytes", stats->nb]];
    [_httrRateLabel setStringValue:[NSString stringWithFormat:@"%ld bytes/s", stats->rate]];
    [_httrLastConnectLabel setStringValue:[formatter stringFromDate:[NSDate dateWithTimeIntervalSince1970: stats->last_connect/1000]]];
    [_httrLastRequestLabel setStringValue:[formatter stringFromDate:[NSDate dateWithTimeIntervalSince1970: stats->last_request/1000]]];

    [formatter release];
}

#pragma mark logique des boutons pause et play, y compris venant de httrack
-(BOOL)coreLogicDownloadWillStart:(CoreLogicDelegate *)sender {
    NSLog(@"Download did start");
    
    [_downloadButton setEnabled:NO];
    [_playpausestopControl setSelectedSegment:HTR_CONTROL_PLAY];
    [_playpausestopControl setEnabled:YES forSegment:HTR_CONTROL_PAUSE];
    [_playpausestopControl setEnabled:YES forSegment:HTR_CONTROL_STOP];
    [_playpausestopControl setEnabled:NO forSegment:HTR_CONTROL_PLAY];
    return YES;
}

-(void)coreLogicDownloadDidStop:(CoreLogic*)sender {
    [_downloadButton setEnabled:YES];
    [_playpausestopControl setSelectedSegment:HTR_CONTROL_STOP];
    [_playpausestopControl setEnabled:NO forSegment:HTR_CONTROL_PAUSE];
    [_playpausestopControl setEnabled:NO forSegment:HTR_CONTROL_STOP];
    [_playpausestopControl setEnabled:NO forSegment:HTR_CONTROL_PLAY];

}

-(void)coreLogicDownloadDidPause:(CoreLogic*)sender {
    [_playpausestopControl setSelectedSegment:HTR_CONTROL_PAUSE];
    [_playpausestopControl setEnabled:NO forSegment:HTR_CONTROL_PAUSE];
    [_playpausestopControl setEnabled:YES forSegment:HTR_CONTROL_STOP];
    [_playpausestopControl setEnabled:YES forSegment:HTR_CONTROL_PLAY];
}

#pragma mark mise a jour de l'outline view
- (void)coreLogicPageAdded:(nonnull CoreLogic *)sender {
    [[self projectsOutlineView] reloadData];
}

- (void)coreLogicDownloadDidAdvance:(nonnull CoreLogic *)sender path:(nonnull NSString *)path domain:(nonnull NSString *)domain ratio:(float)ratio { 
    [[self projectsOutlineView] reloadData];
}

@end

@implementation MonContenuPreview

-(void)mainChangePreview:(NSString*)chemin {
    
    //NSBeginCriticalAlertSheet(@"hello", @"ok?", @"nvm", @"what", [NSApp mainWindow], nil, nil, nil, NULL, @"Un truc s'est mal passé");
    NSAlert * na = [[NSAlert alloc] init];
    //NSDottedFrameRect(NSMakeRect(10, 10, 400, 300));
    //NSDrawButton(<#NSRect rect#>, <#NSRect clipRect#>)
    
    if(0) { /// a garder en tete pour comment envoyer des notifications plus tard
        NSUserNotification* note = [[NSUserNotification alloc] init];
        note.title = @"Salut";
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: note];
        
        NSString* s = @("foo");
        [NSColor underPageBackgroundColor];
        
    }

    for (NSView* v in [self subviews]) {
        [v removeFromSuperview];
    }
    
    if([chemin.pathExtension.lowercaseString isEqualTo:@"html"]) {
        WKWebViewConfiguration* wvc = [[WKWebViewConfiguration alloc] init];
        WKWebView* wv = [[WKWebView alloc] initWithFrame:self.safeAreaRect];
        
        [wv setUIDelegate:self];
        [wv loadRequest:[NSURLRequest requestWithURL:[NSURL fileURLWithPath:chemin]]];
        
        [wvc autorelease];
        [wv autorelease];
        [wv setAutoresizingMask:self.autoresizingMask];
        
        
        [self addSubview:wv];
    }
    
    NSImage* im = [[NSImage alloc] initByReferencingFile:chemin];
    if([im isValid]) {
        NSImageView* iv = [NSImageView imageViewWithImage:im];
        [iv setFrame:NSMakeRect(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, self.frame.size.height)];
        [im autorelease];
        [iv setAutoresizingMask:self.autoresizingMask];
        
        [self addSubview:iv];
    }
    
}

@end


#pragma mark Notre barre de recherche
@interface MySearchInputField: NSSearchField<NSToolbarDelegate>
@end

@implementation MySearchInputField
- (void)controlTextDidChange:(NSNotification *)notification
{
    NSTextView *textView = notification.userInfo[@"NSFieldEditor"];
    
    // prevent calling "complete" too often
    [textView complete:nil];
}


-(void)awakeFromNib
{
    //[self setDelegate:self]; // cause autocompletion incontrolable
    NSSearchFieldCell * cell = [self cell];
    if(@available(macOS 11.0, *)) {
        [cell.searchButtonCell setImage: [NSImage imageWithSystemSymbolName:@"tray.and.arrow.down" accessibilityDescription:@"The URL to download"]];
    }
    //[self complete:nil];
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
    if(item.class == MyDowloadableFile.class) // ou != MyDirectoryElements.class
        return NO;
    NSLevelIndicatorCell * c;
    return item.directories.count || item.files.count;
}

- (NSInteger)outlineView:(nonnull NSOutlineView *)outlineView numberOfChildrenOfItem:(nullable MyDirectoryElements *)item {
    if(item == nil)
        return _delegate.getLogic.websites.directories.count;
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
        else if([tableColumn.identifier isEqual:@"Icone"])
            return nil;
        else raise(42);
    } else if (item.class == MyDowloadableFile.class) {
        MyDowloadableFile * item_cast = (MyDowloadableFile*)item;
        if([tableColumn.identifier isEqual:@"PageName"])
            return item_cast.name;
        else if([tableColumn.identifier isEqual:@"Avancement"])
            return item_cast.downloadAdvancement;
        else if([tableColumn.identifier isEqual:@"Icone"]) {
            NSImage * im = [[NSImage alloc] initByReferencingFile:[NSString stringWithFormat:@"%@/%@", item_cast.hd_path, item_cast.name]];
            
            [im autorelease];
            return im;
        }
        else raise(42);
    }
}

- (nonnull id)outlineView:(nonnull NSOutlineView *)outlineView child:(NSInteger)index ofItem:(nullable MyDirectoryElements*)item {
    if(item == nil)
        return _delegate.getLogic.websites.directories[index];
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

- (void) outlineViewSelectionDidChange:(NSNotification *) notification
{
    if([notification.userInfo objectForKey:@"NSTableViewCurrentRowSelectionUserInfoKey"] == nil)
        return;
    
    NSUInteger idx =((NSIndexSet*)notification.userInfo[@"NSTableViewCurrentRowSelectionUserInfoKey"]).firstIndex;
    
    MyDowloadableFile* orow = [((ProjectsOutlineView*)notification.object) itemAtRow:idx ];
    if([orow respondsToSelector:@selector(hd_path)]) {
        [((ProjectsOutlineView*)notification.object).mainController.contenuPreview mainChangePreview:[NSString stringWithFormat:@"%@/%@", orow.hd_path, orow.name]];
    }
}
@end

#pragma mark ProjectsOutlineView

@implementation ProjectsOutlineView
-(void)awakeFromNib{
    [self reloadData];
}
-(ControllerMainMenu*)mainController {
    return _mainController;
}

@end



NS_ASSUME_NONNULL_END
