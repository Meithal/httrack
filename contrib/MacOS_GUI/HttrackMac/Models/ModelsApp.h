//
//  ModelsApp.h
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SitePageLine : NSObject
@property (copy) NSString * title;
@property (retain) NSMutableArray * children;
-(void) dealloc;
@end

@interface FileSystemItem : NSObject
{
    NSString *relativePath;
    FileSystemItem *parent;
    NSMutableArray *children;
}
 
+ (FileSystemItem *)rootItem;
- (NSInteger)numberOfChildren;// Returns -1 for leaf nodes
- (FileSystemItem *)childAtIndex:(NSUInteger)n; // Invalid to call on leaf nodes
- (NSString *)fullPath;
- (NSString *)relativePath;
 
@end
 

@interface ModelsApp : NSObject

@end

NS_ASSUME_NONNULL_END
