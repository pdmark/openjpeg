/*
 * $Id: metadata_manager.c 44 2011-02-15 12:32:29Z kaori $
 *
 * Copyright (c) 2002-2011, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2011, Professor Benoit Macq
 * Copyright (c) 2010-2011, Kaori Hagihara
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "metadata_manager.h"

#ifdef SERVER
#include "fcgi_stdio.h"
#define logstream FCGI_stdout
#else
#define FCGI_stdout stdout
#define FCGI_stderr stderr
#define logstream stderr
#endif //SERVER


metadatalist_param_t * gene_metadatalist()
{
  metadatalist_param_t *list;

  list = (metadatalist_param_t *)malloc( sizeof(metadatalist_param_t));
  
  list->first = NULL;
  list->last  = NULL;

  return list;
}

metadatalist_param_t * const_metadatalist( int fd)
{
  metadatalist_param_t *metadatalist;
  metadata_param_t *metabin;
  boxlist_param_t *toplev_boxlist;
  box_param_t *box, *next;
  placeholderlist_param_t *phldlist;
  placeholder_param_t *phld;
  int idx;
  struct stat sb;
  
  if( fstat( fd, &sb) == -1){
    fprintf( FCGI_stdout, "Reason: Target broken (fstat error)\r\n");
    return NULL;
  }

  if( !(toplev_boxlist = get_boxstructure( fd, 0, sb.st_size))){
    fprintf( FCGI_stderr, "Error: Not correctl JP2 format\n");
    return NULL;
  }
  
  phldlist = gene_placeholderlist();
  metadatalist = gene_metadatalist();

  delete_box_in_list_by_type( "iptr", toplev_boxlist);
  delete_box_in_list_by_type( "cidx", toplev_boxlist);
  delete_box_in_list_by_type( "fidx", toplev_boxlist);
  
  box = toplev_boxlist->first;
  idx = 0;
  while( box){
    next = box->next;
    if( strncmp( box->type, "jP  ",4)!=0 && strncmp( box->type, "ftyp",4)!=0 && strncmp( box->type, "jp2h",4)!=0){
      boxlist_param_t *boxlist = NULL;
      boxcontents_param_t *boxcontents = NULL;

      phld = gene_placeholder( box, ++idx);
      insert_placeholder_into_list( phld, phldlist);

      boxlist = get_boxstructure( box->fd, get_DBoxoff( box), get_DBoxlen(box));
      if( !boxlist)
	boxcontents = gene_boxcontents( get_DBoxoff( box), get_DBoxlen(box));
      
      delete_box_in_list( &box, toplev_boxlist);
      metabin = gene_metadata( idx, boxlist, NULL, boxcontents);
      insert_metadata_into_list( metabin, metadatalist);
    }
    box = next;
  }

  metabin = gene_metadata( 0, toplev_boxlist, phldlist, NULL);
  insert_metadata_into_list( metabin, metadatalist);

  return metadatalist;
}

void delete_metadatalist( metadatalist_param_t **list)
{
  metadata_param_t *ptr, *next;

  ptr = (*list)->first;

  while( ptr != NULL){
    next=ptr->next;
    delete_metadata( &ptr);
    ptr=next;
  }
  free( *list);
}

metadata_param_t * gene_metadata( int idx, boxlist_param_t *boxlist, placeholderlist_param_t *phldlist, boxcontents_param_t *boxcontents)
{
  metadata_param_t *bin;
  
  bin = (metadata_param_t *)malloc( sizeof(metadata_param_t));
  bin->idx = idx;
  bin->boxlist = boxlist;
  bin->placeholderlist = phldlist;
  bin->boxcontents = boxcontents;
  bin->next = NULL;

  return bin;
}

void delete_metadata( metadata_param_t **metadata)
{
  delete_boxlist( &((*metadata)->boxlist));
  delete_placeholderlist( &((*metadata)->placeholderlist));
  if((*metadata)->boxcontents)
    free((*metadata)->boxcontents);
#ifndef SERVER
  //  fprintf( logstream, "local log: Metadata-bin: %d deleted\n", (*metadata)->idx);
#endif
  free( *metadata);
}

void insert_metadata_into_list( metadata_param_t *metabin, metadatalist_param_t *metadatalist)
{
  if( metadatalist->first)
    metadatalist->last->next = metabin;
  else
    metadatalist->first = metabin;
  metadatalist->last = metabin;
}

void print_metadata( metadata_param_t *metadata)
{
  fprintf( logstream, "metadata-bin %d info:\n", metadata->idx);
  print_allbox( metadata->boxlist);
  print_allplaceholder( metadata->placeholderlist);
 
  boxcontents_param_t *boxcont = metadata->boxcontents;
  if( boxcont)
      fprintf( logstream, "box contents:\n"
	       "\t offset: %lld %#llx\n" 
	       "\t length: %lld %#llx\n", boxcont->offset, boxcont->offset, boxcont->length, boxcont->length);
}

void print_allmetadata( metadatalist_param_t *list)
{
  metadata_param_t *ptr;
  
  fprintf( logstream, "all metadata info: \n");
  ptr = list->first;
  while( ptr != NULL){
    print_metadata( ptr);
    ptr=ptr->next;
  }
}

boxcontents_param_t * gene_boxcontents( Byte8_t offset, Byte8_t length)
{
  boxcontents_param_t *contents;

  contents = (boxcontents_param_t *)malloc( sizeof(boxcontents_param_t));

  contents->offset = offset;
  contents->length = length;

  return contents;
}

metadata_param_t * search_metadata( int idx, metadatalist_param_t *list)
{ 
  metadata_param_t *found;

  found = list->first;
  
  while( found){
    
    if( found->idx == idx)
      return found;
      
    found = found->next;
  }
  return NULL;
}

int search_metadataidx( char boxtype[4], metadatalist_param_t *list)
{
  metadata_param_t *ptr;

  for( int i=0; i<4; i++)
    if( boxtype[i] == '_')
      boxtype[i] = ' ';
  
  ptr = list->first;
  while( ptr){
    if( ptr->boxlist){
      box_param_t *box = ptr->boxlist->first;
      while( box){
	if( strncmp ( boxtype, box->type, 4) == 0)
	  return ptr->idx;
	box = box->next;
      }
    }
    ptr = ptr->next;
  }

  ptr = list->first;
  while( ptr){
    if( ptr->placeholderlist){
      placeholder_param_t *phld = ptr->placeholderlist->first;
      while( phld){
	if( strncmp ( boxtype, (char *)phld->OrigBH+4, 4) == 0){
	  return phld->OrigID;
	}
	phld = phld->next;
      }
    }
    ptr = ptr->next;
  }
  return -1;
}
