#include "main.h"
#include <stdlib.h>

/******************************************************************
	Buffer_Clean
	entfern aus der verketteten Buffereintragsliste diejenigen
	Eintraege, deren DTS kleiner der aktuellen SCR ist. Diese
	Packetdaten sind naemlich bereits dekodiert worden und
	damit aus dem STD elementary stream buffer entfernt.

	Remove entries from FIFO buffer list, if their DTS is 
	less than actual SCR. These packet data have been already
	decoded and have been removed from the system target
	decoder's elementary stream buffer.
******************************************************************/

void buffer_clean (Buffer_struc  *buffer,
					clockticks SCR)
{
    Buffer_queue *pointer;

    while ((buffer->first != NULL) && buffer->first->DTS < SCR)
    {
	  pointer = buffer->first;
	  buffer->first = buffer->first->next;
	  free (pointer);	
    }
}

/******************************************************************
	Buffer_Clean
	entfern aus der verketteten Buffereintragsliste diejenigen
	Eintraege, deren DTS kleiner der aktuellen SCR ist. Diese
	Packetdaten sind naemlich bereits dekodiert worden und
	damit aus dem STD elementary stream buffer entfernt.

	Remove entries from FIFO buffer list, if their DTS is 
	less than actual SCR. These packet data have been already
	decoded and have been removed from the system target
	decoder's elementary stream buffer.
******************************************************************/

void buffer_flush (Buffer_struc  *buffer)
{
    Buffer_queue *pointer;

    while (buffer->first != NULL)
    {
	  pointer = buffer->first;
	  buffer->first = buffer->first->next;
	  free (pointer);	
    }
}

/******************************************************************
	Buffer_Space
	liefert den im Buffer noch freien Platz zurueck.

	returns free space in the buffer
******************************************************************/

int buffer_space (buffer)
Buffer_struc *buffer;
{
    unsigned int used_bytes;
    Buffer_queue *pointer;

    pointer=buffer->first;
    used_bytes=0;

    while (pointer != NULL)
    {
	used_bytes += pointer->size;
	pointer = pointer->next;
    }

    return (buffer->max_size - used_bytes);

}

/******************************************************************
	Queue_Buffer
	fuegt einen Eintrag (bytes, DTS) in der Bufferkette ein.

	adds entry into the buffer FIFO queue
******************************************************************/

void queue_buffer (Buffer_struc *buffer,
					unsigned int bytes,
					clockticks TS)
{
    Buffer_queue *pointer;

    pointer=buffer->first;
    if (pointer==NULL)
    {
	buffer->first = (Buffer_queue*) malloc (sizeof (Buffer_queue));
	buffer->first->size = bytes;
	buffer->first->next=NULL;
	buffer->first->DTS = TS;
    } else
    {
	while ((pointer->next)!=NULL)
	{
	    pointer = pointer->next;
	}
    
	pointer->next = (Buffer_queue*) malloc (sizeof (Buffer_queue));
	pointer->next->size = bytes;
	pointer->next->next = NULL;
	pointer->next->DTS = TS;
    }
}
