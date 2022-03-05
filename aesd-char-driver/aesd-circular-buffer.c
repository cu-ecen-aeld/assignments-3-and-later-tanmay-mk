/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include <syslog.h>     //for logging
#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */

    openlog("aesd_circular_buffer_find_entry_offset_for_fpos", LOG_PID, LOG_USER); 

    struct aesd_buffer_entry *entryptr;
    int idx, i=-1;

    while (++i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        idx = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        entryptr = &(buffer->entry[idx]);
        if (entryptr == NULL)
        {
            syslog(LOG_ERR, "entryptr is NULL\n"); 
            closelog();
            return NULL;
        }

        if (char_offset < entryptr->size) 
        {   
            //offset found
            *entry_offset_byte_rtn = char_offset;
            syslog(LOG_INFO, "Offest found, returning...\n");
            closelog();
            return entryptr; 
        }
        else
        {
            char_offset -= entryptr->size;
        }
    }

    closelog();

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
    
    openlog("aesd_circular_buffer_add_entry", LOG_PID, LOG_USER);

    // check the flag to determine if the buffer is full 
    if(buffer->full == true)
    {
        syslog(LOG_INFO, "Buffer is already full. Overwriting...\n");

        //if buffer is full, overwrite the data
        buffer->entry[buffer->in_offs] = *(add_entry);
        //increment both the head and tail
        buffer->in_offs++;
        buffer->out_offs++;

        closelog();

        return;
    }

    //add new entry to the buffer
    buffer->entry[buffer->in_offs] = *(add_entry);
    buffer->in_offs++;

    //check if write pointer reached limit, if yes, roll back to 0
    if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        buffer->in_offs = 0;
    }

    //Check if buffer is full after writing
    if(buffer->in_offs == buffer->out_offs)
    {
        syslog(LOG_INFO, "Buffer is full after this operation.\n");
        //set the flag to true if buffer is full
        buffer->full = true;
    }
    else
    {   
        syslog(LOG_INFO, "Buffer is not full after this operation.\n");
        //set the flag to false if buffer is not full
        buffer->full = false;
    }

    closelog();
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
