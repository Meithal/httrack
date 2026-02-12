//
//  ControllerMainMenu.m
//  HttrackMac
//
//  Created by ivo on 06/02/2026.
//

#import "ControllerMainMenu.h"
#import "htscore.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NSErrorDomain const MacHttrackErrors = @"com.github.meithal";

static int __cdecl my_loop(t_hts_callbackarg * carg, httrackp * opt, lien_back * back, int back_max, int back_index, int lien_n, int lien_tot, int stat_time, hts_stat_struct * stats) {
    // appelé à chaque boucle de HTTrack
    
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


@implementation ControllerMainMenu

-(void)DownloadSite:(NSString*) url {
    NSLog(@"Controller %@", url);

    httrackp *opt = hts_create_opt();
    htswrap_add(opt, "loop", my_loop);
    htswrap_add(opt, "save-file", my_filesave);
    htswrap_add(opt, "save-file2", my_filesave2);
    htswrap_add(opt, "end", my_end);
    htswrap_add(opt, "free", my_uninit);
    int status = httpmirror([url UTF8String], opt);
    
    if(opt->state.exit_xh != 0) {
        NSString * description = NSLocalizedString(@"Couldn't connect", @"When httpmirror returnn a faulty value");
        NSError * underlyingError = [[NSError alloc] initWithDomain:NSURLErrorDomain code:NSURLErrorBadURL userInfo:nil];
        NSDictionary *errorDictionary = @{ NSLocalizedDescriptionKey : description,
            NSUnderlyingErrorKey : underlyingError, NSURLErrorKey : url };

        [[NSAlert alertWithError:[[NSError alloc] initWithDomain:MacHttrackErrors code:NSURLErrorBadURL userInfo:errorDictionary]] runModal];
    }

}
@end

NS_ASSUME_NONNULL_END
