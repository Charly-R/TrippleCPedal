/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "update.h"
#include "control_chain.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define FIFO_IS_FULL(f)     ((f.head + 1) % CC_UPDATES_FIFO_SIZE == f.tail)
#define FIFO_IS_EMPTY(f)    (f.head == f.tail)


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

typedef struct fifo_t {
    cc_update_t updates[CC_UPDATES_FIFO_SIZE];
    int head, tail;
} fifo_t;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static fifo_t g_updates;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void cc_update_push(const cc_update_t *update)
{
    if (!FIFO_IS_FULL(g_updates))
    {
        int head = g_updates.head;
        g_updates.updates[head].assignment_id = update->assignment_id;
        g_updates.updates[head].value = update->value;

        // increase head
        g_updates.head = (head + 1) % CC_UPDATES_FIFO_SIZE;
    }
}

int cc_update_pop(cc_update_t *update)
{
    if (!FIFO_IS_EMPTY(g_updates))
    {
        int tail = g_updates.tail;
        update->assignment_id = g_updates.updates[tail].assignment_id;
        update->value = g_updates.updates[tail].value;

        // increase tail
        g_updates.tail = (tail + 1) % CC_UPDATES_FIFO_SIZE;

        return 1;
    }

    return 0;
}

int cc_updates_count(void)
{
    if (g_updates.head >= g_updates.tail)
        return ((g_updates.head - g_updates.tail) % CC_UPDATES_FIFO_SIZE);

    return CC_UPDATES_FIFO_SIZE - (g_updates.tail - g_updates.head);
}

void cc_updates_clear(void)
{
    g_updates.head = 0;
    g_updates.tail = 0;
}
