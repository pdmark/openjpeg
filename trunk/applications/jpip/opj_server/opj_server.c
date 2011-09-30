/*
 * $Id: opj_server.c 53 2011-05-09 16:55:39Z kaori $
 *
 * Copyright (c) 2002-2011, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2011, Professor Benoit Macq
 * Copyright (c) 2010-2011, Kaori Hagihara 
 * Copyright (c) 2011,      Lucian Corlaciu, GSoC
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

/*! \file
 *  \brief opj_server is a JPIP server program, which supports HTTP connection, JPT-stream, session, channels, and cache model managements.
 *
 *  \section req Requirements
 *    FastCGI development kit (http://www.fastcgi.com).
 *
 *  \section impinst Implementing instructions
 *  Launch opj_server from the server terminal:\n
 *   % spawn-fcgi -f ./opj_server -p 3000 -n
 *
 *  Note: JP2 files are stored in the working directory of opj_server\n
 *  Check README for the JP2 Encoding\n
 *  
 *  We tested this software with a virtual server running on the same Linux machine as the clients.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "query_parser.h"
#include "channel_manager.h"
#include "session_manager.h"
#include "target_manager.h"
#include "imgreg_manager.h"
#include "msgqueue_manager.h"

#ifndef QUIT_SIGNAL
#define QUIT_SIGNAL "quitJPIP"
#endif

#ifdef SERVER
#include "fcgi_stdio.h"
#define logstream FCGI_stdout
#else
#define FCGI_stdout stdout
#define FCGI_stderr stderr
#define logstream stderr
#endif //SERVER

/**
 * parse JPIP request
 *
 * @param[in]     query_param   structured query
 * @param[in]     sessionlist   session list pointer
 * @param[in]     targetlist    target list pointer
 * @param[in,out] msgqueue      address of the message queue pointer
 * @return                      if succeeded (true) or failed (false)
 */
bool parse_JPIPrequest( query_param_t query_param,
			sessionlist_param_t *sessionlist,
			targetlist_param_t *targetlist,
			msgqueue_param_t **msgqueue);

int main(void)
{ 
  sessionlist_param_t *sessionlist;
  targetlist_param_t *targetlist;
  bool parse_status;
  
  sessionlist = gene_sessionlist();
  targetlist  = gene_targetlist();
  
#ifdef SERVER

  char *query_string;
  while(FCGI_Accept() >= 0)
#else

  char query_string[128];
  while( fgets( query_string, 128, stdin) && query_string[0]!='\n')
#endif
    {

#ifdef SERVER     
      query_string = getenv("QUERY_STRING");    
#endif //SERVER

      if( strcmp( query_string, QUIT_SIGNAL) == 0)
	break;
           
      query_param_t query_param;
      msgqueue_param_t *msgqueue;

      parse_query( query_string, &query_param); 
      
      switch( query_param.return_type){
      case JPPstream:
	fprintf( FCGI_stdout, "Content-type: image/jpp-stream\r\n");
	break;
      default:
	fprintf( FCGI_stdout, "Content-type: image/jpt-stream\r\n");
	break;
      }

#ifndef SERVER
      print_queryparam( query_param);
#endif
      
      msgqueue = NULL;
      if( !(parse_status = parse_JPIPrequest( query_param, sessionlist, targetlist, &msgqueue)))
	fprintf( FCGI_stderr, "Error: JPIP request failed\n");
            
      fprintf( FCGI_stdout, "\r\n");

#ifndef SERVER
      //      if( parse_status)
      // 	print_allsession( sessionlist);
      print_msgqueue( msgqueue);
#endif

      emit_stream_from_msgqueue( msgqueue);

      delete_msgqueue( &msgqueue);
    }
  
  fprintf( FCGI_stderr, "JPIP server terminated by a client request\n");

  delete_sessionlist( &sessionlist);
  delete_targetlist( &targetlist);

  return 0;
}

/**
 * REQUEST: target identification by target or tid request
 *
 * @param[in]     query_param   structured query
 * @param[in]     targetlist    target list pointer
 * @param[out]    target        address of target pointer
 * @return                      if succeeded (true) or failed (false)
 */
bool identify_target( query_param_t query_param, targetlist_param_t *targetlist, target_param_t **target);

/**
 * REQUEST: channel association
 *          this must be processed before any process
 *
 * @param[in]     query_param   structured query
 * @param[in]     sessionlist   session list pointer
 * @param[out]    cursession    address of the associated session pointer
 * @param[out]    curchannel    address of the associated channel pointer
 * @return                      if succeeded (true) or failed (false)
 */
bool associate_channel( query_param_t    query_param, 
			sessionlist_param_t *sessionlist,
			session_param_t **cursession, 
			channel_param_t **curchannel);
/**
 * REQUEST: new channel (cnew) assignment
 *
 * @param[in]     query_param   structured query
 * @param[in]     sessionlist   session list pointer
 * @param[in]     target        requested target pointer
 * @param[in,out] cursession    address of the associated/opened session pointer
 * @param[in,out] curchannel    address of the associated/opened channel pointer
 * @return                      if succeeded (true) or failed (false)
 */
bool open_channel( query_param_t query_param, 
		   sessionlist_param_t *sessionlist,
		   target_param_t *target,
		   session_param_t **cursession, 
		   channel_param_t **curchannel);

/**
 * REQUEST: channel close (cclose)
 *
 * @param[in]     query_param   structured query
 * @param[in]     sessionlist   session list pointer
 * @param[in,out] cursession    address of the session pointer of deleting channel
 * @param[in,out] curchannel    address of the deleting channel pointer
 * @return                      if succeeded (true) or failed (false)
 */
bool close_channel( query_param_t query_param, 
		    sessionlist_param_t *sessionlist,
		    session_param_t **cursession, 
		    channel_param_t **curchannel);

/**
 * REQUEST: view-window (fsiz)
 *
 * @param[in]     query_param structured query
 * @param[in]     target      requested target pointer
 * @param[in,out] cursession  associated session pointer
 * @param[in,out] curchannel  associated channel pointer
 * @param[out]    msgqueue    address of the message queue pointer
 * @return                    if succeeded (true) or failed (false)
 */
bool gene_JPIPstream( query_param_t query_param,
		     target_param_t *target,
		     session_param_t *cursession, 
		     channel_param_t *curchannel,
		     msgqueue_param_t **msgqueue);

bool parse_JPIPrequest( query_param_t query_param,
			sessionlist_param_t *sessionlist,
			targetlist_param_t *targetlist,
			msgqueue_param_t **msgqueue)
{ 
  target_param_t *target = NULL;
  session_param_t *cursession = NULL;
  channel_param_t *curchannel = NULL;

  if( query_param.target[0] != '\0' || query_param.tid[0] != '\0'){
    if( !identify_target( query_param, targetlist, &target))
      return false;
  }

  if( query_param.cid[0] != '\0'){
    if( !associate_channel( query_param, sessionlist, &cursession, &curchannel))
      return false;
  }

  if( query_param.cnew){
    if( !open_channel( query_param, sessionlist, target, &cursession, &curchannel))
      return false;
  }
  if( query_param.cclose[0][0] != '\0')
    if( !close_channel( query_param, sessionlist, &cursession, &curchannel))
      return false;
  
  if( (query_param.fx > 0 && query_param.fy > 0) || query_param.box_type[0][0] != 0)
    if( !gene_JPIPstream( query_param, target, cursession, curchannel, msgqueue))
      return false;
  
  return true;
}

bool identify_target( query_param_t query_param, targetlist_param_t *targetlist, target_param_t **target)
{
  if( query_param.tid[0] !='\0' && strcmp( query_param.tid, "0") != 0 ){
    if( query_param.cid[0] != '\0'){
      fprintf( FCGI_stdout, "Reason: Target can not be specified both through tid and cid\r\n");
      fprintf( FCGI_stdout, "Status: 400\r\n");
      return false;
    }
    if( ( *target = search_targetBytid( query_param.tid, targetlist)))
      return true;
  }

  if( query_param.target[0] !='\0')
    if( !( *target = search_target( query_param.target, targetlist)))
      if(!( *target = gene_target( targetlist, query_param.target)))
	return false;

  if( *target){
    fprintf( FCGI_stdout, "JPIP-tid: %s\r\n", (*target)->tid);
    return true;
  }
  else{
    fprintf( FCGI_stdout, "Reason: target not found\r\n");
    fprintf( FCGI_stdout, "Status: 400\r\n");
    return false;
  }
}

bool associate_channel( query_param_t    query_param, 
			sessionlist_param_t *sessionlist,
			session_param_t **cursession, 
			channel_param_t **curchannel)
{
  if( search_session_and_channel( query_param.cid, sessionlist, cursession, curchannel)){
    
    if( !query_param.cnew)
      set_channel_variable_param( query_param, *curchannel);
  }
  else{
    fprintf( FCGI_stderr, "Error: process canceled\n");
    return false;
  }
  return true;
}

bool open_channel( query_param_t query_param, 
		   sessionlist_param_t *sessionlist,
		   target_param_t *target,
		   session_param_t **cursession, 
		   channel_param_t **curchannel)
{
  cachemodel_param_t *cachemodel = NULL;

  if( target){
    if( !(*cursession))
      *cursession = gene_session( sessionlist);
    if( !( cachemodel = search_cachemodel( target, (*cursession)->cachemodellist)))
      if( !(cachemodel = gene_cachemodel( (*cursession)->cachemodellist, target)))
	return false;
  }
  else
    if( *curchannel)
      cachemodel = (*curchannel)->cachemodel;

  *curchannel = gene_channel( query_param, cachemodel, (*cursession)->channellist);
  if( *curchannel == NULL)
    return false;

  return true;
}

bool close_channel( query_param_t query_param, 
		    sessionlist_param_t *sessionlist,
		    session_param_t **cursession, 
		    channel_param_t **curchannel)
{
  if( query_param.cclose[0][0] =='*'){
#ifndef SERVER
    fprintf( logstream, "local log: close all\n");
#endif
    // all channels associatd with the session will be closed
    if( !delete_session( cursession, sessionlist))
      return false;
  }
  else{
    // check if all entry belonging to the same session
    int i=0;
    while( query_param.cclose[i][0] !='\0'){
      
      // In case of the first entry of close cid
      if( *cursession == NULL){
	if( !search_session_and_channel( query_param.cclose[i], sessionlist, cursession, curchannel))
	  return false;
      }
      else // second or more entry of close cid
	if( !(*curchannel=search_channel( query_param.cclose[i], (*cursession)->channellist))){
	  fprintf( FCGI_stdout, "Reason: Cclose id %s is from another session\r\n", query_param.cclose[i]); 
	  return false;
	}
      i++;
    }
    // delete channels
    i=0;
    while( query_param.cclose[i][0] !='\0'){
      
      *curchannel = search_channel( query_param.cclose[i], (*cursession)->channellist);
      delete_channel( curchannel, (*cursession)->channellist);
      i++;
    }
    
    if( (*cursession)->channellist->first == NULL || (*cursession)->channellist->last == NULL)
      // In case of empty session
      delete_session( cursession, sessionlist);
  }
  return true;
}


/**
 * enqueue tiles or precincts into the message queue
 *
 * @param[in] query_param structured query
 * @param[in] msgqueue    message queue pointer  
 */
void enqueue_imagedata( query_param_t query_param, msgqueue_param_t *msgqueue);

/**
 * enqueue metadata bins into the message queue
 *
 * @param[in]     query_param  structured query
 * @param[in]     metadatalist pointer to metadata bin list
 * @param[in,out] msgqueue     message queue pointer  
 */
void enqueue_metabins( query_param_t query_param, metadatalist_param_t *metadatalist, msgqueue_param_t *msgqueue);


bool gene_JPIPstream( query_param_t query_param,
		      target_param_t *target,
		      session_param_t *cursession, 
		      channel_param_t *curchannel,
		      msgqueue_param_t **msgqueue)
{
  index_param_t *codeidx;
  cachemodel_param_t *cachemodel;
  
  if( !cursession || !curchannel){ // stateless
    if( !target)
      return false;
    if( !(cachemodel = gene_cachemodel( NULL, target)))
      return false;
    *msgqueue = gene_msgqueue( true, cachemodel);
  }
  else{ // session
    cachemodel  = curchannel->cachemodel;
    target = cachemodel->target;
    *msgqueue = gene_msgqueue( false, cachemodel);
  }
  
  codeidx = target->codeidx;

  //meta
  if( query_param.box_type[0][0] != 0)
    enqueue_metabins( query_param, codeidx->metadatalist, *msgqueue); 

  // image codestream
  if( query_param.fx > 0 && query_param.fy > 0){
    if( !cachemodel->mhead_model)
      enqueue_mainheader( *msgqueue);
    enqueue_imagedata( query_param, *msgqueue);
  }
  return true;
}


/**
 * enqueue precinct data-bins into the queue
 *
 * @param[in] xmin      min x coordinate in the tile at the decomposition level
 * @param[in] xmax      max x coordinate in the tile at the decomposition level
 * @param[in] ymin      min y coordinate in the tile at the decomposition level
 * @param[in] ymax      max y coordinate in the tile at the decomposition level
 * @param[in] tile_id   tile index
 * @param[in] level     decomposition level
 * @param[in] lastcomp  last component number
 * @param[in] comps     pointer to the array that stores the requested components
 * @param[in] msgqueue  message queue
 * @return
 */
void enqueue_precincts( int xmin, int xmax, int ymin, int ymax, int tile_id, int level, int lastcomp, bool *comps, msgqueue_param_t *msgqueue);

/**
 * enqueue all precincts inside a tile into the queue
 *
 * @param[in] tile_id   tile index
 * @param[in] level     decomposition level
 * @param[in] lastcomp  last component number
 * @param[in] comps     pointer to the array that stores the requested components
 * @param[in] msgqueue  message queue
 * @return
 */
void enqueue_allprecincts( int tile_id, int level, int lastcomp, bool *comps, msgqueue_param_t *msgqueue);

void enqueue_imagedata( query_param_t query_param, msgqueue_param_t *msgqueue)
{
  index_param_t *codeidx;
  imgreg_param_t imgreg;
  range_param_t tile_Xrange, tile_Yrange;
  int u, v, tile_id;
  int xmin, xmax, ymin, ymax;

  codeidx = msgqueue->cachemodel->target->codeidx;

  imgreg  = map_viewin2imgreg( query_param.fx, query_param.fy, 
			       query_param.rx, query_param.ry, query_param.rw, query_param.rh,
			       codeidx->SIZ.XOsiz, codeidx->SIZ.YOsiz, codeidx->SIZ.Xsiz, codeidx->SIZ.Ysiz, 
			       codeidx->COD.numOfdecomp+1);

  for( u=0, tile_id=0; u<codeidx->SIZ.YTnum; u++){
    tile_Yrange = get_tile_Yrange( codeidx->SIZ, tile_id, imgreg.level);
    
    for( v=0; v<codeidx->SIZ.XTnum; v++, tile_id++){
      tile_Xrange = get_tile_Xrange( codeidx->SIZ, tile_id, imgreg.level);
	
      if( tile_Xrange.minvalue < tile_Xrange.maxvalue && tile_Yrange.minvalue < tile_Yrange.maxvalue){
	if( tile_Xrange.maxvalue <= imgreg.xosiz + imgreg.ox || 
	    tile_Xrange.minvalue >= imgreg.xosiz + imgreg.ox + imgreg.sx ||
	    tile_Yrange.maxvalue <= imgreg.yosiz + imgreg.oy || 
	    tile_Yrange.minvalue >= imgreg.yosiz + imgreg.oy + imgreg.sy) {
	  //printf("Tile completely excluded from view-window %d\n", tile_id);
	  // Tile completely excluded from view-window
	}
	else if( tile_Xrange.minvalue >= imgreg.xosiz + imgreg.ox && 
		 tile_Xrange.maxvalue <= imgreg.xosiz + imgreg.ox + imgreg.sx && 
		 tile_Yrange.minvalue >= imgreg.yosiz + imgreg.oy && 
		 tile_Yrange.maxvalue <= imgreg.yosiz + imgreg.oy + imgreg.sy) {
	  // Tile completely contained within view-window
	  // high priority
	  //printf("Tile completely contained within view-window %d\n", tile_id);
	  if( query_param.return_type == JPPstream){
	    enqueue_tileheader( tile_id, msgqueue);
	    enqueue_allprecincts( tile_id, imgreg.level, query_param.lastcomp, query_param.comps, msgqueue);
	  }
	  else
	    enqueue_tile( tile_id, imgreg.level, msgqueue);
	}
	else{
	  // Tile partially overlaps view-window
	  // low priority
	  //printf("Tile partially overlaps view-window %d\n", tile_id);
	  if( query_param.return_type == JPPstream){
	    enqueue_tileheader( tile_id, msgqueue);
	    xmin = tile_Xrange.minvalue >= imgreg.xosiz + imgreg.ox ? 0 : imgreg.xosiz + imgreg.ox - tile_Xrange.minvalue;
	    xmax = tile_Xrange.maxvalue <= imgreg.xosiz + imgreg.ox + imgreg.sx ? tile_Xrange.maxvalue - tile_Xrange.minvalue -1 : imgreg.xosiz + imgreg.ox + imgreg.sx - tile_Xrange.minvalue -1;
	    ymin = tile_Yrange.minvalue >= imgreg.yosiz + imgreg.oy ? 0 : imgreg.yosiz + imgreg.oy - tile_Yrange.minvalue;
	    ymax = tile_Yrange.maxvalue <= imgreg.yosiz + imgreg.oy + imgreg.sy ? tile_Yrange.maxvalue - tile_Yrange.minvalue -1 : imgreg.yosiz + imgreg.oy + imgreg.sy - tile_Yrange.minvalue -1;
	    enqueue_precincts( xmin, xmax, ymin, ymax, tile_id, imgreg.level, query_param.lastcomp, query_param.comps, msgqueue);
	  }
	  else
	    enqueue_tile( tile_id, imgreg.level, msgqueue);
	}
      }
    }
  }
}


void enqueue_precincts( int xmin, int xmax, int ymin, int ymax, int tile_id, int level, int lastcomp, bool *comps, msgqueue_param_t *msgqueue)
{
  index_param_t *codeidx;
  int c, u, v, res_lev, dec_lev;
  int seq_id;
  Byte4_t XTsiz, YTsiz;
  Byte4_t XPsiz, YPsiz;
  Byte4_t xminP, xmaxP, yminP, ymaxP;

  codeidx  = msgqueue->cachemodel->target->codeidx;

  for( c=0; c<codeidx->SIZ.Csiz; c++)
    if( lastcomp == -1 /*all*/ || ( c<=lastcomp && comps[c])){
      seq_id = 0;
      for( res_lev=0, dec_lev=codeidx->COD.numOfdecomp; dec_lev>=level; res_lev++, dec_lev--){
	
	XTsiz = get_tile_XSiz( codeidx->SIZ, tile_id, dec_lev);
	YTsiz = get_tile_YSiz( codeidx->SIZ, tile_id, dec_lev);
	
	XPsiz = codeidx->COD.XPsiz[ res_lev];
	YPsiz = codeidx->COD.YPsiz[ res_lev];
	
	for( u=0; u<ceil((double)YTsiz/(double)YPsiz); u++){
	  yminP = u*YPsiz;
	  ymaxP = (u+1)*YPsiz-1;
	  if( YTsiz <= ymaxP)
	    ymaxP = YTsiz-1;
	  
	  for( v=0; v<ceil((double)XTsiz/(double)XPsiz); v++, seq_id++){
	    xminP = v*XPsiz;
	    xmaxP = (v+1)*XPsiz-1;
	    if( XTsiz <= xmaxP)
	      xmaxP = XTsiz-1;
	    
	    if( xmaxP < xmin || xminP > xmax || ymaxP < ymin || yminP > ymax){
	      // Precinct completely excluded from view-window
	    }
	    else if( xminP >= xmin && xmaxP <= xmax && yminP >= ymin && ymaxP <= ymax){
	      // Precinct completely contained within view-window
	      // high priority
	      enqueue_precinct( seq_id, tile_id, c, msgqueue);
	    }
	    else{
	      // Precinct partially overlaps view-window
	      // low priority
	      enqueue_precinct( seq_id, tile_id, c, msgqueue);
	    }
	  }
	}
      }
    }
}

void enqueue_allprecincts( int tile_id, int level, int lastcomp, bool *comps, msgqueue_param_t *msgqueue)
{
  index_param_t *codeidx;
  int c, i, res_lev, dec_lev;
  int seq_id;
  Byte4_t XTsiz, YTsiz;
  Byte4_t XPsiz, YPsiz;

  codeidx  = msgqueue->cachemodel->target->codeidx;

  for( c=0; c<codeidx->SIZ.Csiz; c++)
    if( lastcomp == -1 /*all*/ || ( c<=lastcomp && comps[c])){
      seq_id = 0;
      for( res_lev=0, dec_lev=codeidx->COD.numOfdecomp; dec_lev>=level; res_lev++, dec_lev--){
	
	XTsiz = get_tile_XSiz( codeidx->SIZ, tile_id, dec_lev);
	YTsiz = get_tile_YSiz( codeidx->SIZ, tile_id, dec_lev);

	XPsiz = codeidx->COD.XPsiz[ res_lev];
	YPsiz = codeidx->COD.YPsiz[ res_lev];
	
	for( i=0; i<ceil((double)YTsiz/(double)YPsiz)*ceil((double)XTsiz/(double)XPsiz); i++, seq_id++){
	  enqueue_precinct( seq_id, tile_id, c, msgqueue);
	}
      }
    }
}

void enqueue_metabins( query_param_t query_param, metadatalist_param_t *metadatalist, msgqueue_param_t *msgqueue)
{
  int i;
  for( i=0; query_param.box_type[i][0]!=0 && i<MAX_NUMOFBOX; i++){
    if( query_param.box_type[i][0] == '*'){
      // not implemented
    }
    else{
      int idx = search_metadataidx( query_param.box_type[i], metadatalist);

      if( idx != -1)
	enqueue_metadata( idx, msgqueue);
    }
  }
}
