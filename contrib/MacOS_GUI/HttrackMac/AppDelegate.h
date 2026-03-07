
#import <Cocoa/Cocoa.h>

#import "CoreLogic.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    CoreLogic* _logic;
}
// MARK: overrides
-(void)changeWindowSubtitle:(NSString*)newSubtitle;
// MARK: ours
-(void)warnUser:(NSString*)description domain:(NSErrorDomain) domain code:(NSInteger) code; /* show a popup warning */
-(CoreLogic*)getLogic;
@end

