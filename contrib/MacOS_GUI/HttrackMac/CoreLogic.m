//
//  CoreLogic.m
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import "CoreLogic.h"
#import "AppDelegate.h"
#import "Models/ModelsApp.h"

#import "htscore.h"

NS_ASSUME_NONNULL_BEGIN


NSErrorDomain const MacHttrackErrors = @"com.github.meithal";

#pragma mark fonctions bridge httrack
static int __cdecl my_loop(t_hts_callbackarg * carg, httrackp * opt, lien_back * back, int back_max, int back_index, int lien_n, int lien_tot, int stat_time, hts_stat_struct * stats) {
    // appelé à chaque boucle de HTTrack, permet d'arreter un telechargement
    // si besoin
    if(CALLBACKARG_PREV_FUN(carg, loop) != NULL) {
        CALLBACKARG_PREV_FUN(carg, loop)(CALLBACKARG_PREV_CARG( carg), opt, back, back_max, back_index, lien_n, lien_tot, stat_time, stats);
    }
    
    //printf("loop lien :%s \n");
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        if(![[NSApp delegate] respondsToSelector:@selector(getLogic)]){
            return;
        }
        
        CoreLogic* logic = [((AppDelegate*)[NSApp delegate]) getLogic];

        httrackp * opt = [logic httrack_opt];
        enum CoreLogicState old_state = [logic state];
        if(opt) {
            if(opt->state.stop) {
                if(old_state != CORELOGIC_STATE_STOPPED)
                    [[logic delegate] coreLogicDownloadDidStop:logic];
                [logic setState:CORELOGIC_STATE_STOPPED];
            }
            else if(opt->state._hts_setpause) {
                if(old_state != CORELOGIC_STATE_PAUSED)
                    [[logic delegate] coreLogicDownloadDidPause:logic];
                [logic setState:CORELOGIC_STATE_PAUSED];
            } else {
                if(old_state != CORELOGIC_STATE_RUNNING)
                    [[logic delegate] coreLogicDownloadWillStart:logic];
                [logic setState:CORELOGIC_STATE_RUNNING];
            }
        }
        
        if([logic loopCallback]) { // met a jour l'affichage des stats
            [[logic objCallback] performSelector:[logic loopCallback] withObject:(id)stats];
        }
        
        if(back && back->r.totalsize > 0) {
            float ratio = (float)back->r.size / back->r.totalsize;
            [[logic websites] updateAdvancement:@(back->url_fil) site:@(back->url_adr) ratio:ratio];
            [[logic delegate] coreLogicDownloadDidAdvance:logic path:@(back->url_fil) domain:@(back->url_adr) ratio:ratio];
        }
    }];

    return 1;
}

static void __cdecl my_filesave(t_hts_callbackarg * carg,
                               httrackp * opt, const char *file) {
    // Appellé après avoir sauvegardé un fichier
    
    printf("TOTO my_filesave %s\n", file);
    
    return;
}

static void __cdecl my_filesave2(
     t_hts_callbackarg * carg,
     httrackp * opt, const char *adr,
     const char *file, const char *sav,
     int is_new, int is_modified,
     int not_updated) {
    // Appellé avant de sauvegarder un fichier
    
    printf("TOTO2 my_filesave2 adr: %s file: %s sac: %s is new %d is modified %d not updated %d\n", adr, file, sav, is_new, is_modified, not_updated);
    
    for(int i=0; i < opt->lien_tot; i++) {
        printf("lien %d: %s\n", i, opt->liens[i]->sav);
    }
    
//    if(file[0] == '\0') // empty file
//        return;
    
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        CoreLogic* logic = [((AppDelegate*)[NSApp delegate]) getLogic];

        httrackp * opt = [logic httrack_opt];
        for(int i=0; i<logic.websites.directories.count; i++) {
            if([logic.websites.directories[i].name isEqualToString:@(adr)]) {
                [ModelsApp addFile:@(file) toArborescence:logic.websites.directories[i] sittingAt:[NSString stringWithCString:adr encoding:NSUnicodeStringEncoding]];
                break;
            }
        }
        
        [[logic delegate] coreLogicPageAdded:logic];
    }];
    
    return;
}

static int __cdecl my_end(
    t_hts_callbackarg * carg, httrackp * opt) {
    printf("TOTO my_end\n");
    /* call parent functions if multiple callbacks are chained. you can skip this part, if you don't want previous callbacks to be called. */
    if (CALLBACKARG_PREV_FUN(carg, end) != NULL) {
      /* status is ok on our side, return other callabck's status */
      return CALLBACKARG_PREV_FUN(carg, end)(CALLBACKARG_PREV_CARG(carg), opt);
    }

    return 1;
}

static int __cdecl my_linkdetected(t_hts_callbackarg * carg,
                                           httrackp * opt, char *link) {
    printf("TOTOLINK my_linkdetected %s\n", link);
  return 1;
}

#pragma mark fonction coeur de metier

void parseDirectoriesRecurse(MyDirectoryElements * dir, NSURL * adress);
void parseDirectoriesRecurse(MyDirectoryElements * dir, NSURL * adress)
{
    NSDirectoryEnumerator * dirEn = [NSFileManager.defaultManager enumeratorAtPath:[adress path]];
    
    NSString * file;
    while ((file = [dirEn nextObject])) {
        [dirEn skipDescendants];
        BOOL isDir = NO;
        if([NSFileManager.defaultManager fileExistsAtPath:[[adress path] stringByAppendingPathComponent:file] isDirectory:&isDir])
        {
            if(isDir) {
                parseDirectoriesRecurse([ModelsApp addDirectory:file toArborescene:dir], [adress URLByAppendingPathComponent:file]);
            } else {
                [ModelsApp addFile:file toArborescence:dir sittingAt:[adress path]];
            }
        }
    }
}

#pragma mark CoreLogic
@implementation CoreLogic

-(id)init{
    self = [super init];
    
    if (self) {
        
        [self initHttrack];
        _eventDispatcher = [[HtmrEventDispatcher alloc] init];
        _state = CORELOGIC_STATE_STOPPED;
    }
    
    return self;
}

- (void)dealloc
{
    hts_free_opt(_httrack_opt);
    [_websites release];
    [_eventDispatcher release];
    
    [super dealloc];
}

-(void)setDelegate:(nullable CoreLogicDelegate*)newDelegate {
    _delegate = newDelegate;
}

-(CoreLogicDelegate*)delegate {
    return _delegate;
}

-(void)setLoopCallback:(nullable SEL) callback withObject:(nullable id) obj {
    _loopCallback = callback;
    _objCallback = obj;
}
-(SEL)loopCallback {
    return _loopCallback;
}
-(id)objCallback {
    return _objCallback;
}
-(nullable httrackp*)httrack_opt {
    return _httrack_opt;;
}

#pragma mark initHttrack
-(void)initHttrack {
    _httrack_opt = hts_create_opt();
    _httrack_opt->debug = LOG_ERROR;
    //_httrack_opt->log = stderr;
    
    // On recupere le HOME sur mac
    NSArray<NSURL *> * urls = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask];
    NSURL * url = urls.firstObject;
    url = [NSURL URLWithString:@"Mirrored Websites/" relativeToURL:url];

    StringMemcpy(_httrack_opt->path_html, ([[url path] stringByAppendingString:@"/"]).UTF8String, [url path].length + 1);
    StringCopyS(_httrack_opt->path_log, _httrack_opt->path_html);
    StringCopyN(_httrack_opt->path_html_utf8, StringBuff(_httrack_opt->path_html),
                      StringLength(_httrack_opt->path_html));
    
    htswrap_add(_httrack_opt, "loop", my_loop);
    htswrap_add(_httrack_opt, "save-file", my_filesave);
    htswrap_add(_httrack_opt, "save-file2", my_filesave2);
    htswrap_add(_httrack_opt, "end", my_end);
    htswrap_add(_httrack_opt, "link-detected", my_linkdetected);
}

-(MyDirectoryElements *) websites
{
    if(_websites == nil) {
        _websites = [[MyDirectoryElements createFromString:@"racine"] retain];
        [self indexOfDownloadedSites:_websites];
    }
    
    return _websites;
}

-(void)dowloadSite:(NSString*) url onError:(void (^)(NSString *, NSErrorDomain, NSInteger code)) onError
{
    NSBlockOperation * operation = [NSBlockOperation blockOperationWithBlock:^{
        hts_init(); // ensure that openSSLctx is initialized
        int status = httpmirror([url UTF8String], _httrack_opt);
        
        if(_httrack_opt->state.exit_xh != 0) {
            NSString * description = NSLocalizedString(@"Couldn't connect", @"When httpmirror return a faulty value");
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                onError(description, MacHttrackErrors, NSURLErrorBadURL);
            }];
        }

        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [_delegate coreLogicDownloadDidStop:self];
        }];
    }];
    
    NSOperationQueue * queue = [[NSOperationQueue alloc] init];
    [queue setName:@"Httrack download queue"];
    
    if(![_delegate coreLogicDownloadWillStart:self]) { // our delegate callback
        return;
    }
    [operation setName:@"Httrack download operation"];
    
    [queue addOperation:operation];

    //[operation autorelease];
    [queue autorelease];
}

-(void)indexOfDownloadedSites:(MyDirectoryElements *) arbo
{
    NSURL * url = [NSURL URLWithString:@"Mirrored Websites/" relativeToURL:[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject];
    
    NSDirectoryEnumerator * dirEn = [NSFileManager.defaultManager enumeratorAtPath:[url path]];
    
    NSString * file;
    while ((file = [dirEn nextObject])) {
        [dirEn skipDescendants];
        BOOL isDir = NO;
        if([NSFileManager.defaultManager fileExistsAtPath:[[url path] stringByAppendingPathComponent:file] isDirectory:&isDir] && isDir)
        {
            if([NSFileManager.defaultManager fileExistsAtPath:[[[url path] stringByAppendingPathComponent:file] stringByAppendingPathComponent:@"index.html"]]) {
                
                parseDirectoriesRecurse([ModelsApp addDirectory:file toArborescene:arbo], [url URLByAppendingPathComponent:file]);
                
            }
        }
    }
    
    return;
}

-(void)pauseMirror:(int)p {
    hts_setpause(_httrack_opt, p);
}

-(void)stopMirror {
    hts_request_stop(_httrack_opt, 0);
}

-(enum CoreLogicState) state {
    return _state;
}
-(void)setState:(enum CoreLogicState)state {
    _state = state;
}

@end

#pragma mark HtmrEventDispatcher
@implementation HtmrEventDispatcher

- (BOOL)removeEventListener:(nonnull void (^)(void))fun {
}

- (void)addEventListener:(nonnull void (^)(void))fun {
}

@end

NS_ASSUME_NONNULL_END
