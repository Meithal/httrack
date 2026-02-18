//
//  CoreLogic.m
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//
#import "CoreLogic.h"
#import "Models/ModelsApp.h"

#import "htscore.h"

NS_ASSUME_NONNULL_BEGIN


NSErrorDomain const MacHttrackErrors = @"com.github.meithal";

#pragma mark fonctions bridge httrack
static int __cdecl my_loop(t_hts_callbackarg * carg, httrackp * opt, lien_back * back, int back_max, int back_index, int lien_n, int lien_tot, int stat_time, hts_stat_struct * stats) {
    // appelé à chaque boucle de HTTrack, permet d'arreter un telechargement
    // si besoin
    
    //printf("loop\n");
  return 1;
}

static void __cdecl my_filesave(t_hts_callbackarg * carg,
                               httrackp * opt, const char *file) {
    // Appellé après avoir sauvegardé un fichier
    
    printf("TOTO %s\n", file);
  return;
}

static void __cdecl my_filesave2(
     t_hts_callbackarg * carg,
     httrackp * opt, const char *adr,
     const char *file, const char *sav,
     int is_new, int is_modified,
     int not_updated) {
    // Appellé avant de sauvegarder un fichier
    
    printf("TOTO2 %s\n", file);
  return;
}

static int __cdecl my_end(
    t_hts_callbackarg * carg, httrackp * opt) {
  return 1;
}

static void __cdecl my_uninit(t_hts_callbackarg * carg) {
  // hts_freevar();
    
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
                [ModelsApp addFile:file toArborescence:dir];
            }
        }
    }
}

#pragma mark CoreLogic
@implementation CoreLogic
-(MyDirectoryElements *) websites
{
    if(_websites == nil) {
        _websites = [[MyDirectoryElements createFromString:@"racine"] retain];
        [self indexOfDownloadedSites:_websites];
    }
    
    return _websites;
}
@synthesize websites = _websites;

-(void)dowloadSite:(NSString*) url onError:(void (^)(NSDictionary *, NSErrorDomain)) onError
{
    
    // TODO: implement download cancellation and pause
    // - https://stackoverflow.com/questions/8113268/how-to-cancel-nsblockoperation
    NSBlockOperation * operation = [NSBlockOperation blockOperationWithBlock:^{
        int status = httpmirror([url UTF8String], _httrack_opt);
        
        if(_httrack_opt->state.exit_xh != 0) {
            NSString * description = NSLocalizedString(@"Couldn't connect", @"When httpmirror returnn a faulty value");
            
            NSError * underlyingError = [[NSError alloc] initWithDomain:NSURLErrorDomain code:NSURLErrorBadURL userInfo:nil];
            
            NSDictionary * errorDictionary = @{
                NSLocalizedDescriptionKey : description,
                NSUnderlyingErrorKey : underlyingError,
                NSURLErrorKey : url
            };
            [underlyingError autorelease];

            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                onError(errorDictionary, MacHttrackErrors);
            }];
        }
    }];
    
    NSOperationQueue * queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
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

-(id)init{
    self = [super init];
    
    if (self) {
        NSLog(@"init core appellé");
        
        _httrack_opt = hts_create_opt();
        
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
        htswrap_add(_httrack_opt, "free", my_uninit);
    }
    
    return self;
}

- (void)dealloc
{
    hts_free_opt(_httrack_opt);
    [_websites release];
    
    [super dealloc];
}
@end

NS_ASSUME_NONNULL_END
