//
//  CoreLogic.m
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//
#import <stdio.h>

#import "CoreLogic.h"

#import "htscore.h"
#import "htstools.h"
#include "htslib.h"

NS_ASSUME_NONNULL_BEGIN


NSErrorDomain const MacHttrackErrors = @"com.github.meithal";

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

@implementation CoreLogic
-(void)dowloadSite:(NSString*) url
{
    
    // TODO: implement download cancellation
    // - https://stackoverflow.com/questions/8113268/how-to-cancel-nsblockoperation
    NSBlockOperation * operation = [NSBlockOperation blockOperationWithBlock:^{
        int status = httpmirror([url UTF8String], _httrack_opt);
        
        if(_httrack_opt->state.exit_xh != 0) {
            NSString * description = NSLocalizedString(@"Couldn't connect", @"When httpmirror returnn a faulty value");
            NSError * underlyingError = [[NSError alloc] initWithDomain:NSURLErrorDomain code:NSURLErrorBadURL userInfo:nil];
            NSDictionary *errorDictionary = @{ NSLocalizedDescriptionKey : description,
                NSUnderlyingErrorKey : underlyingError, NSURLErrorKey : url };

            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self.app warnUser:[[NSError alloc] initWithDomain:MacHttrackErrors code:NSURLErrorBadURL userInfo:errorDictionary]];
            }];
        }
    }];
    
    NSOperationQueue * queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];

}

-(NSArray *)indexOfDownloadedSites
{
    puts("foo");
    
    char path_buffer[FILENAME_MAX];
    snprintf(path_buffer, FILENAME_MAX, "%s%s", _httrack_opt->path_log.buffer_, ".");
    find_handle h = hts_findfirst(path_buffer);
    if(h) {
        do{
            if(hts_findisdir(h)) {
                //fexist();
                char path_buffer[FILENAME_MAX];
                snprintf(path_buffer, FILENAME_MAX, "%s%s", _httrack_opt->path_log.buffer_, hts_findgetname(h));
                find_handle h2 = hts_findfirst(path_buffer);
            }
            printf("%s\n", hts_findgetname(h));
        } while(hts_findnext(h));
    }
    hts_findclose(h);
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
    
    [super dealloc];
}
@end

NS_ASSUME_NONNULL_END
