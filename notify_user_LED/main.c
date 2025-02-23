#include <klee/klee.h>
#include <malloc.h>
#include <assert.h>
#include <stdint.h>

#define BIT_SET(src, bit) ((src & bit) == bit)
#define USER_PROCESS_BOOTUP (1 << 8)
#define YELLOW 6

const size_t pUserData_size = 4240; // size of UserDataBuffer

extern int processLists;
extern void* pUserData;
extern uint8_t ledColor; // bit trick

extern void notify_user_LED();

int main() {
    klee_make_symbolic(&processLists, sizeof(processLists), "processLists");
    pUserData = malloc(pUserData_size);

    notify_user_LED();
    assert(!BIT_SET(processLists, USER_PROCESS_BOOTUP) || ledColor == YELLOW);
    return 0;
}