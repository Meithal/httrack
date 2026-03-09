#include <pthread/pthread.h>

#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
    }
    
    printf("api min %d, api max %d\n", __MAC_OS_X_VERSION_MAX_ALLOWED,
           __MAC_OS_X_VERSION_MIN_REQUIRED);
    printf("is mutithreaded %d\n", [NSThread isMultiThreaded]);
    
    // Source - https://stackoverflow.com/a/6829140
    // Posted by kennytm
    // Retrieved 2026-03-09, License - CC BY-SA 3.0

    pthread_t self = pthread_self();
    void* addr = pthread_get_stackaddr_np(self);
    size_t size = pthread_get_stacksize_np(self);
    printf("addr=%p size=%zx\n", addr, size);

    
    //NSLog(@"GC %@\n", [NSGarbageCollector defaultCollector]);

    return NSApplicationMain(argc, argv);
}
