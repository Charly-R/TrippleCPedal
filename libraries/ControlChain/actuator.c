/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "control_chain.h"
#include "actuator.h"
#include "update.h"
#include <math.h>


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define MAX_ACTUATORS   (CC_MAX_DEVICES * CC_MAX_ACTUATORS)


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


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static cc_actuator_t g_actuators[MAX_ACTUATORS];
static unsigned int g_actuators_count;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static int momentary_process(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    float actuator_value = *(actuator->value);
    if (actuator_value > 0.0)
    {
        if (actuator->lock == 0)
        {
            actuator->lock = 1;

            // option list mode
#ifdef CC_OPTIONS_LIST_SUPPORTED
            if (assignment->mode & CC_MODE_OPTIONS)
            {
                assignment->list_index++;

                if (assignment->list_index >= assignment->list_count)
                    assignment->list_index = 0;

                assignment->value = assignment->list_items[assignment->list_index]->value;
            }
            else
#endif

            // trigger mode
            if (assignment->mode & CC_MODE_TRIGGER)
            {
                assignment->value = assignment->max;
            }

            // toggle mode
            else if (assignment->mode & CC_MODE_TOGGLE)
            {
                assignment->value = 1.0 - assignment->value;
            }

            return 1;
        }
    }
    else
    {
        actuator->lock = 0;
    }

    return 0;
}

static int continuos_process(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    float actuator_value = *(actuator->value);

    // check if actuator value has changed the minimum required value
    float delta = (actuator->max + actuator->min) * 0.01;
    if (fabs(actuator->last_value - actuator_value) < delta)
        return 0;

    // update value
    actuator->last_value = actuator_value;

    // toggle and trigger modes
    if (assignment->mode & CC_MODE_TOGGLE || assignment->mode & CC_MODE_TRIGGER)
    {
        float middle = (actuator->max + actuator->min) / 2.0;

        if (actuator_value >= middle)
        {
            assignment->value = 1.0;
        }
        else if (assignment->mode & CC_MODE_TOGGLE)
        {
            assignment->value = 0.0;
        }
        else
        {
            return 0;
        }

        return 1;
    }

    // option list mode
#ifdef CC_OPTIONS_LIST_SUPPORTED
    else if (assignment->mode & CC_MODE_OPTIONS)
    {
        float step = (actuator->max + actuator->min) / (float) assignment->list_count;

        assignment->list_index = actuator_value / step;

        if (assignment->list_index >= assignment->list_count)
            assignment->list_index = assignment->list_count - 1;

        assignment->value = assignment->list_items[assignment->list_index]->value;

        return 1;
    }
#endif

    float a, b;
    a = (assignment->max - assignment->min) / (actuator->max - actuator->min);
    b = assignment->min - a*actuator->min;

    float value = a*actuator_value + b;

    // real mode
    if (assignment->mode & CC_MODE_REAL)
    {
        assignment->value = value;
        return 1;
    }

    // integer mode
    else if (assignment->mode & CC_MODE_INTEGER)
    {
        assignment->value = roundf(value);
        return 1;
    }

    return 0;
}

static int update_assignment_value(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    switch (actuator->type)
    {
        case CC_ACTUATOR_MOMENTARY:
            return momentary_process(actuator, assignment);

        case CC_ACTUATOR_CONTINUOUS:
            return continuos_process(actuator, assignment);
    }

    return 0;
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

cc_actuator_t *cc_actuator_new(cc_actuator_config_t *config)
{
    if (g_actuators_count >= MAX_ACTUATORS)
        return 0;

    cc_actuator_t *actuator = &g_actuators[g_actuators_count];

    // initialize actuator data struct
    actuator->id = g_actuators_count;
    actuator->type = config->type;
    actuator->value = config->value;
    actuator->min = config->min;
    actuator->max = config->max;
    actuator->supported_modes = config->supported_modes;
    actuator->max_assignments = config->max_assignments;
    str16_create(config->name, &actuator->name);

    g_actuators_count++;

    return actuator;
}

void cc_actuator_map(cc_assignment_t *assignment)
{
    // link assignment to actuator
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = assignment;
            break;
        }
    }

#ifdef CC_OPTIONS_LIST_SUPPORTED
    // initialize option list index
    if (assignment->mode & CC_MODE_OPTIONS)
    {
        for (int i = 0; i < assignment->list_count; i++)
        {
            if (assignment->value == assignment->list_items[i]->value)
            {
                assignment->list_index = i;
                break;
            }
        }
    }
#endif
}

void cc_actuator_unmap(cc_assignment_t *assignment)
{
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = 0;
            break;
        }
    }
}

void cc_actuators_process(void (*events_cb)(void *arg))
{
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
        cc_assignment_t *assignment = actuator->assignment;

        if (!assignment)
            continue;

        // update assignment value according current actuator value
        int updated = update_assignment_value(actuator, assignment);
        if (updated)
        {
            // append update to be sent
            cc_update_t update;
            update.assignment_id = assignment->id;
            update.value = assignment->value;
            cc_update_push(&update);

            if (events_cb)
            {
                cc_event_t event;
                event.id = CC_EV_UPDATE;
                event.data = assignment;
                events_cb(&event);
            }
        }
    }
}
