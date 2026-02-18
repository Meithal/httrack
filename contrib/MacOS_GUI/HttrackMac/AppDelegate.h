
#import <Cocoa/Cocoa.h>

#import "CoreLogic.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    CoreLogic* _logic;
}
-(CoreLogic*)getLogic;
// ours
-(void)changeWindowSubtitle:(NSString*)newSubtitle;
-(void)warnUser:(NSError*)error; /* show a popup warning */
@end

