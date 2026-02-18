//
//  ModelsApp.m
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import "ModelsApp.h"

NS_ASSUME_NONNULL_BEGIN

@implementation MyDowloadableFile
+ (nonnull instancetype)createFromString:(nonnull NSString *)name {
    MyDowloadableFile * me =[[MyDowloadableFile alloc] init];
    me.name = name;
    return [me autorelease];
}
@end

@implementation MyDirectoryElements
+(instancetype) createFromString:(NSString *) name {
    MyDirectoryElements * me =[[MyDirectoryElements alloc] init];
    me.name = name;
    me.directories = [[NSMutableArray alloc] init];
    me.files = [[NSMutableArray alloc] init];
    return [me autorelease];
}
//-(void)dealloc {
//    [super dealloc];
//    
//    //[self.files release];
//    //[self.directories release];
//}
@end

@implementation ModelsApp
+(MyDirectoryElements *)allocArborescence
{
    MyDirectoryElements * dir = [[MyDirectoryElements alloc] init];
    dir.depth = 0;
    return dir;
}

+ (nonnull id)addFile:(nonnull NSString *)file toArborescence:(nonnull MyDirectoryElements *)arbo {
    [arbo.files addObject:[MyDowloadableFile createFromString:file]];
    
    return self;
}

+ (nonnull MyDirectoryElements *)addDirectory:(nonnull NSString *)dirname toArborescene:(nonnull MyDirectoryElements *)arbo {
    MyDirectoryElements *n =[MyDirectoryElements createFromString:dirname];
    n.depth = arbo.depth + 1;
    [arbo.directories addObject:n];
    return n;
}

@end

NS_ASSUME_NONNULL_END
