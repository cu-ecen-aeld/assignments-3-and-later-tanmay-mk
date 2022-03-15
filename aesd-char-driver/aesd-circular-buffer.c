/**
 * @file        aesd-circular-buffer.c
 * @brief       Functions and data related to a circular buffer imlementation
 *
 * @author      Dan Walkes
 *              - Updated by Tanmay Mahendra Kothale on 5th March 2022
 *              - Changes for ECEN 5713 Advanced Embedded Software
 *                Development Assignment 7 Part 1.  
 *
 *              - Updated by Tanmay Mahendra Kothale on 13th March 2022
 *              - Changes for ECEN 5713 Advanced Embedded Software
 *                Development Assignment 8.             
 * 
 * @date        2020-03-01
 * @copyright   Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

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

    struct aesd_buffer_entry *entryptr;
    
    int idx, i=-1;

    while (++i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        idx = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        entryptr = &(buffer->entry[idx]);
        if (entryptr == NULL)
        {
            return NULL;
        }

        if (char_offset < entryptr->size) 
        {   
            //offset found
            *entry_offset_byte_rtn = char_offset;
            return entryptr; 
        }
        else
        {
            char_offset -= entryptr->size;
        }
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */

    const char *ptr = NULL;
    

    // check the flag to determine if the buffer is full 
    if((buffer->full == true) && (buffer->in_offs == buffer -> out_offs))
    {

        ptr = buffer->entry[buffer->in_offs].buffptr;

        //if buffer is full, overwrite the data
        buffer->entry[buffer->in_offs] = *(add_entry);
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        //check if write pointer reached limit, if yes, roll back to 0
        if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            buffer->in_offs = 0;
        }
        buffer->out_offs = buffer->in_offs; 
    }

    else
    {

        //add new entry to the buffer
        buffer->entry[buffer->in_offs] = *(add_entry);
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        //check if write pointer reached limit, if yes, roll back to 0
        if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            buffer->in_offs = 0;
        }

        //Check if buffer is full after writing
        if(buffer->in_offs == buffer->out_offs)
        {
            //set the flag to true if buffer is full
            buffer->full = true;
        }
        else
        {   
            //set the flag to false if buffer is not full
            buffer->full = false;
        }
        
    }

    return ptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
