#import "MainMenu.h"

NS_ASSUME_NONNULL_BEGIN

@implementation MainMenu
- (IBAction)httrDowloadButton:(id)sender {
    NSLog(@"Push %@", [self.httrSiteUrl stringValue]);
}


@end

NS_ASSUME_NONNULL_END
