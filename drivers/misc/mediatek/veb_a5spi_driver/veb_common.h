#ifndef __VEB_COMMON_H__
#define __VEB_COMMON_H__

#include <linux/string.h>
#include <linux/spi/spi.h>

#define DRIVER_NAME "[veb_a5_spi]"
#define DRIVER_PREFIX DRIVER_NAME ": "

#define VEB_DBG(fmt, args...)   \
    printk(KERN_DEBUG DRIVER_PREFIX "%5d: <%s>" fmt, __LINE__, __func__, ##args)


#define VEB_TRACE_IN()          VEB_DBG("IN\n")
#define VEB_TRACE_OUT()         VEB_DBG("OUT\n")


#define VEB_INF(fmt, args...)   \
    printk(KERN_INFO DRIVER_PREFIX "%5d: <%s>" fmt, __LINE__,__func__,##args)
#define VEB_WRN(fmt, args...)   \
    printk(KERN_WARNING DRIVER_PREFIX "%5d: <%s>" fmt, __LINE__,__func__,##args)
#define VEB_ERR(fmt, args...)   \
    printk(KERN_ERR DRIVER_PREFIX "%5d: <%s>" fmt, __LINE__,__func__,##args)

#ifdef CFG_VEB_DEBUG
#define VEB_DDUMP(buffer, length)   \
    do{                                                     \
        print_hex_dump(KERN_DEBUG, DRIVER_PREFIX,           \
                DUMP_PREFIX_OFFSET, 16, 1,                  \
                buffer,                                     \
                min_t(size_t, length, 1024),                \
                true);                                      \
    }while(0)

#define VEB_DDUMP_WITH_PREFIX(prefix, buffer, length)   \
    do{                                                     \
        VEB_DBG("%s\n", prefix);                            \
        print_hex_dump(KERN_DEBUG, DRIVER_PREFIX,           \
                DUMP_PREFIX_OFFSET, 16, 1,                  \
                buffer,                                     \
                min_t(size_t, length, 1024),                \
                true);                                      \
    }while(0)
#else
#define VEB_DDUMP(buffer, length)   \
    do{}while(0)
#define VEB_DDUMP_WITH_PREFIX(prefix, buffer, length)   \
    do{}while(0)
#endif

#define VEB_EDUMP(buffer, length)   \
    do{                                                     \
        print_hex_dump(KERN_ERR, DRIVER_PREFIX,             \
                DUMP_PREFIX_OFFSET, 16, 1,                  \
                buffer,                                     \
                min_t(size_t, length, 1024),                \
                true);                                      \
    }while(0)

#define VEB_EDUMP_WITH_PREFIX(prefix, buffer, length)   \
    do{                                                     \
        VEB_ERR("%s\n", prefix);                            \
        print_hex_dump(KERN_DEBUG, DRIVER_PREFIX,           \
                DUMP_PREFIX_OFFSET, 16, 1,                  \
                buffer,                                     \
                min_t(size_t, length, 1024),                \
                true);                                      \
    }while(0)

#define MIN(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#define MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))
#endif
