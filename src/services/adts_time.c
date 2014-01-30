
#include <time.h>  /* clock_gettime() */
#include <sched.h> /* sched_getcpu() */
#include <string.h>
#include <inttypes.h>

#include <adts_services.h>


/******************************************************************************
 #####  ####### ######  #     #  #####  ####### #     # ######  #######  #####
#     #    #    #     # #     # #     #    #    #     # #     # #       #     #
#          #    #     # #     # #          #    #     # #     # #       #
 #####     #    ######  #     # #          #    #     # ######  #####    #####
      #    #    #   #   #     # #          #    #     # #   #   #             #
#     #    #    #    #  #     # #     #    #    #     # #    #  #       #     #
 #####     #    #     #  #####   #####     #     #####  #     # #######  #####
******************************************************************************/


/*
 ****************************************************************************
 * \brief
 *
 * \detail
 *   Attempt to align the entries to cache affinity interval
 *
 ****************************************************************************
 */
#define TSTAMP_MESSAGE_BYTES (64)
#define TSTAMP_WORKSPACE_BYTES (128)
#define TSTAMP_ENTRIES (TSTAMP_WORKSPACE_BYTES / sizeof(struct timespec))

/*
 ****************************************************************************
 * \brief
 *
 * \detail
 *
 *
 ****************************************************************************
 */
typedef struct {
    int64_t         mim;
    int64_t         max;
    int64_t         last;
    int32_t         elems_curr;
    int32_t         elems_limit;
    struct timespec history[ TSTAMP_ENTRIES ];
    char            message[ TSTAMP_MESSAGE_BYTES ];
} tstamp_mgr_t;



/******************************************************************************
 * ####### #     # #     #  #####  #######   ###   ####### #     #  #####
 * #       #     # ##    # #     #    #       #    #     # ##    # #     #
 * #       #     # # #   # #          #       #    #     # # #   # #
 * #####   #     # #  #  # #          #       #    #     # #  #  #  #####
 * #       #     # #   # # #          #       #    #     # #   # #       #
 * #       #     # #    ## #     #    #       #    #     # #    ## #     #
 * #        #####  #     #  #####     #      ###   ####### #     #  #####
******************************************************************************/

//ts_init()
//ts_start()
//ts_stop()
//ts_add()
//ts_calc()
//ts_get_last()
//ts_get_min()
//ts_get_max()
//ts_display()



/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
int64_t
adts_timestamp_approximate( void )
{
    uint64_t         tsval  = 0;
    struct timespec  ts     = {0};
    struct timespec *p_ts   = &(ts);

    clock_gettime(CLOCK_REALTIME, p_ts);
    tsval = p_ts->tv_sec + p_ts->tv_nsec;

    return tsval;
} /* adts_timestamp_approximate() */


/******************************************************************************
 * #     # #     #   ###   ####### ####### #######  #####  #######  #####
 * #     # ##    #    #       #       #    #       #     #    #    #     #
 * #     # # #   #    #       #       #    #       #          #    #
 * #     # #  #  #    #       #       #    #####    #####     #     #####
 * #     # #   # #    #       #       #    #             #    #          #
 * #     # #    ##    #       #       #    #       #     #    #    #     #
 *  #####  #     #   ###      #       #    #######  #####     #     #####
******************************************************************************/

/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
static void
utest_control( void )
{
    CDISPLAY("=========================================================");
    {
        int64_t ts = 0;

        ts = adts_timestamp_approximate();
        CDISPLAY("0x%016llu", ts);
    }

    return;
} /* utest_control() */


/*
 ****************************************************************************
 * test entrypoint
 *
 ****************************************************************************
 */
void
utest_adts_time( void )
{

    utest_control();

    return;
} /* utest_adts_time() */
