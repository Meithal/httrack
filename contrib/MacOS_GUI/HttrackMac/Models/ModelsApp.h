//
//  ModelsApp.h
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MyDowloadableFile : NSObject
@property (copy) NSString * name;
@property (assign) NSNumber * downloadAdvancement; /** 0.0 - 1.0 */
@property (assign) NSDate * dateCreated;
@property (copy) NSString * hd_path;
+(instancetype) createFromString:(NSString *) name atPath:(NSString*) path;
@end

@interface MyDirectoryElements : NSObject
@property (copy) NSString* name;
@property (strong, atomic) NSMutableArray<MyDirectoryElements *> * directories;
@property (strong, atomic) NSMutableArray<MyDowloadableFile *> * files;
@property NSInteger depth;

+(instancetype) createFromString:(NSString *) name;
-(BOOL) updateAdvancement:(NSString*)path site:(NSString*)site ratio:(float)ratio;
@end

@interface ModelsApp : NSObject
+(MyDirectoryElements *)allocArborescence;
+(MyDirectoryElements*)addDirectory:(NSString *)dirname toArborescene:(MyDirectoryElements *)arbo;
+(id)addFile:(NSString*) file toArborescence:(MyDirectoryElements *)arbo sittingAt:(NSString*) path;
@end

NS_ASSUME_NONNULL_END
