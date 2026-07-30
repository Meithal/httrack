//
//  ModelsApp.m
//  HttrackMac
//
//  Created by ivo on 13/02/2026.
//

#import "ModelsApp.h"

NS_ASSUME_NONNULL_BEGIN

@implementation MyDowloadableFile
+ (nonnull instancetype)createFromString:(nonnull NSString *)name
atPath:(nonnull NSString *)path {
    MyDowloadableFile * me =[[MyDowloadableFile alloc] init];
    me.name = name;
    me.hd_path = path;
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


- (BOOL)updateAdvancement:(nonnull NSString *)path site:(NSString*)site ratio:(float)ratio {
    NSLog(@"me: %@ path: %@ site: %@ ratio: %.2f\n", _name, path, site, ratio);
    for(int i=0; i<self.directories.count; i++) {
        if([self.directories[i].name isEqualToString:site]) {
            for(int j=0;j<self.directories[i].directories.count; j++)
                if([self.directories[i].files[j].name isEqualToString:path])
                    self.directories[i].files[j].downloadAdvancement = @(ratio);            
            break;
        }
    }
}

@end

@implementation ModelsApp
+(MyDirectoryElements *)allocArborescence
{
    MyDirectoryElements * dir = [[MyDirectoryElements alloc] init];
    dir.depth = 0;
    return dir;
}

+ (nonnull id)addFile:(nonnull NSString *)file toArborescence:(nonnull MyDirectoryElements *)arbo sittingAt:(NSString*) path {
    [arbo.files addObject:[MyDowloadableFile createFromString:file atPath:path]];
    
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
