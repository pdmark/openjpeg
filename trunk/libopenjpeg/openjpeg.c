/*
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
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

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "opj_config.h"
#include "opj_includes.h"


/**
 * Decompression handler.
 */
typedef struct opj_decompression
{
	/** Main header reading function handler*/
	opj_bool (*opj_read_header) (	struct opj_stream_private * cio,
									void * p_codec,
									opj_image_t **p_image,
									struct opj_event_mgr * p_manager);
	/** Decoding function */
	opj_bool (*opj_decode) (	void * p_codec,
								struct opj_stream_private *p_cio,
								opj_image_t *p_image,
								struct opj_event_mgr * p_manager);
	/** FIXME DOC */
	opj_bool (*opj_read_tile_header)(	void * p_codec,
										OPJ_UINT32 * p_tile_index,
										OPJ_UINT32* p_data_size,
										OPJ_INT32 * p_tile_x0, OPJ_INT32 * p_tile_y0,
										OPJ_INT32 * p_tile_x1, OPJ_INT32 * p_tile_y1,
										OPJ_UINT32 * p_nb_comps,
										opj_bool * p_should_go_on,
										struct opj_stream_private *p_cio,
										struct opj_event_mgr * p_manager);
	/** FIXME DOC */
	opj_bool (*opj_decode_tile_data)(	void * p_codec,
										OPJ_UINT32 p_tile_index,
										OPJ_BYTE * p_data,
										OPJ_UINT32 p_data_size,
										struct opj_stream_private *p_cio,
										struct opj_event_mgr * p_manager);
	/** Reading function used after codestream if necessary */
	opj_bool (* opj_end_decompress) (	void *p_codec,
										struct opj_stream_private *cio,
										struct opj_event_mgr * p_manager);
	/** Codec destroy function handler*/
	void (*opj_destroy) (void * p_codec);
	/** Setup decoder function handler */
	void (*opj_setup_decoder) (void * p_codec, opj_dparameters_t * p_param);
	/** Set decode area function handler */
	opj_bool (*opj_set_decode_area) (	void * p_codec,
										opj_image_t* p_image,
										OPJ_INT32 p_start_x, OPJ_INT32 p_end_x,
										OPJ_INT32 p_start_y, OPJ_INT32 p_end_y,
										struct opj_event_mgr * p_manager);

	/** Get tile function */
	opj_bool (*opj_get_decoded_tile) (	void *p_codec,
										opj_stream_private_t *p_cio,
										opj_image_t *p_image,
										struct opj_event_mgr * p_manager,
										OPJ_UINT32 tile_index);

	/** Set the decoded resolution factor */
	opj_bool (*opj_set_decoded_resolution_factor) (void * p_codec, OPJ_UINT32 res_factor, struct opj_event_mgr * p_manager);

}opj_decompression_t;

/**
 * Compression handler. FIXME DOC
 */
typedef struct opj_compression
{
	opj_bool (* opj_start_compress) (	void *p_codec,
										struct opj_stream_private *cio,
										struct opj_image * p_image,
										struct opj_event_mgr * p_manager);

	opj_bool (* opj_encode) (	void * p_codec,
								struct opj_stream_private *p_cio,
								struct opj_event_mgr * p_manager);

	opj_bool (* opj_write_tile) (	void * p_codec,
									OPJ_UINT32 p_tile_index,
									OPJ_BYTE * p_data,
									OPJ_UINT32 p_data_size,
									struct opj_stream_private * p_cio,
									struct opj_event_mgr * p_manager);

	opj_bool (* opj_end_compress) (	void * p_codec,
									struct opj_stream_private *p_cio,
									struct opj_event_mgr * p_manager);

	void (* opj_destroy) (void * p_codec);

	void (*opj_setup_encoder) (	void * p_codec,
								opj_cparameters_t * p_param,
								struct opj_image * p_image,
								struct opj_event_mgr * p_manager);

}opj_compression_t;

/**
 * Main codec handler used for compression or decompression.
 */
typedef struct opj_codec_private
{
	/** FIXME DOC */
	union 
    {
        opj_decompression_t m_decompression;
        opj_compression_t m_compression;
    } m_codec_data;
    /** FIXME DOC*/
	void * m_codec;
	/** Event handler */
	opj_event_mgr_t m_event_mgr;
	/** Flag to indicate if the codec is used to decode or encode*/
	opj_bool is_decompressor;
	void (*opj_dump_codec) (void * p_codec, OPJ_INT32 info_flag, FILE* output_stream);
	opj_codestream_info_v2_t* (*opj_get_codec_info)(void* p_codec);
	opj_codestream_index_t* (*opj_get_codec_index)(void* p_codec);
}
opj_codec_private_t;

/* ---------------------------------------------------------------------- */
/**
 * Default callback function.
 * Do nothing.
 */
void opj_default_callback (const char *msg, void *client_data)
{
}

void set_default_event_handler(opj_event_mgr_t * p_manager)
{
	p_manager->m_error_data = 00;
	p_manager->m_warning_data = 00;
	p_manager->m_info_data = 00;
	p_manager->error_handler = opj_default_callback;
	p_manager->info_handler = opj_default_callback;
	p_manager->warning_handler = opj_default_callback;
}

opj_bool OPJ_CALLCONV opj_set_info_handler(	opj_codec_t * p_codec, 
											opj_msg_callback p_callback,
											void * p_user_data)
{
	opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
	if(! l_codec){
		return OPJ_FALSE;
	}
	
	l_codec->m_event_mgr.info_handler = p_callback;
	l_codec->m_event_mgr.m_info_data = p_user_data;
	
	return OPJ_TRUE;
}

opj_bool OPJ_CALLCONV opj_set_warning_handler(	opj_codec_t * p_codec, 
												opj_msg_callback p_callback,
												void * p_user_data)
{
	opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
	if (! l_codec) {
		return OPJ_FALSE;
	}
	
	l_codec->m_event_mgr.warning_handler = p_callback;
	l_codec->m_event_mgr.m_warning_data = p_user_data;
	
	return OPJ_TRUE;
}

opj_bool OPJ_CALLCONV opj_set_error_handler(opj_codec_t * p_codec, 
											opj_msg_callback p_callback,
											void * p_user_data)
{
	opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
	if (! l_codec) {
		return OPJ_FALSE;
	}
	
	l_codec->m_event_mgr.error_handler = p_callback;
	l_codec->m_event_mgr.m_error_data = p_user_data;
	
	return OPJ_TRUE;
}

/* ---------------------------------------------------------------------- */


OPJ_SIZE_T opj_read_from_file (void * p_buffer, OPJ_SIZE_T p_nb_bytes, FILE * p_file)
{
	OPJ_SIZE_T l_nb_read = fread(p_buffer,1,p_nb_bytes,p_file);
	return l_nb_read ? l_nb_read : -1;
}

OPJ_UINT64 opj_get_data_length_from_file (FILE * p_file)
{
	OPJ_OFF_T file_length = 0;

	OPJ_FSEEK(p_file, 0, SEEK_END);
	file_length = (OPJ_UINT64)OPJ_FTELL(p_file);
	OPJ_FSEEK(p_file, 0, SEEK_SET);

	return file_length;
}

OPJ_SIZE_T opj_write_from_file (void * p_buffer, OPJ_SIZE_T p_nb_bytes, FILE * p_file)
{
	return fwrite(p_buffer,1,p_nb_bytes,p_file);
}

OPJ_OFF_T opj_skip_from_file (OPJ_OFF_T p_nb_bytes, FILE * p_user_data)
{
	if (OPJ_FSEEK(p_user_data,p_nb_bytes,SEEK_CUR)) {
		return -1;
	}

	return p_nb_bytes;
}

opj_bool opj_seek_from_file (OPJ_OFF_T p_nb_bytes, FILE * p_user_data)
{
	if (OPJ_FSEEK(p_user_data,p_nb_bytes,SEEK_SET)) {
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

/* ---------------------------------------------------------------------- */
#ifdef _WIN32
#ifndef OPJ_STATIC
BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	OPJ_ARG_NOT_USED(lpReserved);
	OPJ_ARG_NOT_USED(hModule);

	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH :
			break;
		case DLL_PROCESS_DETACH :
			break;
		case DLL_THREAD_ATTACH :
		case DLL_THREAD_DETACH :
			break;
    }

    return TRUE;
}
#endif /* OPJ_STATIC */
#endif /* _WIN32 */

/* ---------------------------------------------------------------------- */


const char* OPJ_CALLCONV opj_version(void) {
    return PACKAGE_VERSION;
}




opj_codec_t* OPJ_CALLCONV opj_create_decompress_v2(OPJ_CODEC_FORMAT p_format)
{
	opj_codec_private_t *l_codec = 00;

	l_codec = (opj_codec_private_t*) opj_calloc(1, sizeof(opj_codec_private_t));
	if (!l_codec){
		return 00;
	}
	memset(l_codec, 0, sizeof(opj_codec_private_t));

	l_codec->is_decompressor = 1;

	switch (p_format) {
		case CODEC_J2K:
			l_codec->opj_dump_codec = (void (*) (void*, OPJ_INT32, FILE*)) j2k_dump;

			l_codec->opj_get_codec_info = (opj_codestream_info_v2_t* (*) (void*) ) j2k_get_cstr_info;

			l_codec->opj_get_codec_index = (opj_codestream_index_t* (*) (void*) ) j2k_get_cstr_index;

			l_codec->m_codec_data.m_decompression.opj_decode =
					(opj_bool (*) (	void *,
									struct opj_stream_private *,
									opj_image_t*, struct opj_event_mgr * )) j2k_decode_v2;

			l_codec->m_codec_data.m_decompression.opj_end_decompress =
					(opj_bool (*) (	void *,
									struct opj_stream_private *,
									struct opj_event_mgr *)) j2k_end_decompress;

			l_codec->m_codec_data.m_decompression.opj_read_header =
					(opj_bool (*) (	struct opj_stream_private *,
									void *,
									opj_image_t **,
									struct opj_event_mgr * )) j2k_read_header;

			l_codec->m_codec_data.m_decompression.opj_destroy =
					(void (*) (void *))j2k_destroy;

			l_codec->m_codec_data.m_decompression.opj_setup_decoder =
					(void (*) (void * , opj_dparameters_t * )) j2k_setup_decoder_v2;

			l_codec->m_codec_data.m_decompression.opj_read_tile_header =
					(opj_bool (*) (	void *,
									OPJ_UINT32*,
									OPJ_UINT32*,
									OPJ_INT32*, OPJ_INT32*,
									OPJ_INT32*, OPJ_INT32*,
									OPJ_UINT32*,
									opj_bool*,
									struct opj_stream_private *,
									struct opj_event_mgr * )) j2k_read_tile_header;

			l_codec->m_codec_data.m_decompression.opj_decode_tile_data =
					(opj_bool (*) (void *, OPJ_UINT32, OPJ_BYTE*, OPJ_UINT32, struct opj_stream_private *, struct opj_event_mgr *)) j2k_decode_tile;

			l_codec->m_codec_data.m_decompression.opj_set_decode_area =
					(opj_bool (*) (void *, opj_image_t*, OPJ_INT32, OPJ_INT32, OPJ_INT32, OPJ_INT32, struct opj_event_mgr *)) j2k_set_decode_area;

			l_codec->m_codec_data.m_decompression.opj_get_decoded_tile = (opj_bool (*) (void *p_codec,
																						opj_stream_private_t *p_cio,
																						opj_image_t *p_image,
																						struct opj_event_mgr * p_manager,
																						OPJ_UINT32 tile_index)) j2k_get_tile;

			l_codec->m_codec_data.m_decompression.opj_set_decoded_resolution_factor = (opj_bool (*) (void * p_codec,
																									OPJ_UINT32 res_factor,
																									struct opj_event_mgr * p_manager)) j2k_set_decoded_resolution_factor;

			l_codec->m_codec = j2k_create_decompress_v2();

			if (! l_codec->m_codec) {
				opj_free(l_codec);
				return NULL;
			}

			break;

		case CODEC_JP2:
			/* get a JP2 decoder handle */
			l_codec->opj_dump_codec = (void (*) (void*, OPJ_INT32, FILE*)) jp2_dump;

			l_codec->opj_get_codec_info = (opj_codestream_info_v2_t* (*) (void*) ) jp2_get_cstr_info;

			l_codec->opj_get_codec_index = (opj_codestream_index_t* (*) (void*) ) jp2_get_cstr_index;

			l_codec->m_codec_data.m_decompression.opj_decode =
					(opj_bool (*) (	void *,
									struct opj_stream_private *,
									opj_image_t*,
									struct opj_event_mgr * )) jp2_decode_v2;

			l_codec->m_codec_data.m_decompression.opj_end_decompress =  (opj_bool (*) (void *,struct opj_stream_private *,struct opj_event_mgr *)) jp2_end_decompress;

			l_codec->m_codec_data.m_decompression.opj_read_header =  (opj_bool (*) (
					struct opj_stream_private *,
					void *,
					opj_image_t **,
					struct opj_event_mgr * )) jp2_read_header;

			l_codec->m_codec_data.m_decompression.opj_read_tile_header = ( opj_bool (*) (
					void *,
					OPJ_UINT32*,
					OPJ_UINT32*,
					OPJ_INT32*,
					OPJ_INT32*,
					OPJ_INT32 * ,
					OPJ_INT32 * ,
					OPJ_UINT32 * ,
					opj_bool *,
					struct opj_stream_private *,
					struct opj_event_mgr * )) jp2_read_tile_header;

			l_codec->m_codec_data.m_decompression.opj_decode_tile_data = (opj_bool (*) (void *,OPJ_UINT32,OPJ_BYTE*,OPJ_UINT32,struct opj_stream_private *,	struct opj_event_mgr * )) jp2_decode_tile;

			l_codec->m_codec_data.m_decompression.opj_destroy = (void (*) (void *))jp2_destroy;

			l_codec->m_codec_data.m_decompression.opj_setup_decoder = (void (*) (void * ,opj_dparameters_t * )) jp2_setup_decoder_v2;

			l_codec->m_codec_data.m_decompression.opj_set_decode_area = (opj_bool (*) (void *,opj_image_t*, OPJ_INT32,OPJ_INT32,OPJ_INT32,OPJ_INT32, struct opj_event_mgr * )) jp2_set_decode_area;

			l_codec->m_codec_data.m_decompression.opj_get_decoded_tile = (opj_bool (*) (void *p_codec,
																						opj_stream_private_t *p_cio,
																						opj_image_t *p_image,
																						struct opj_event_mgr * p_manager,
																						OPJ_UINT32 tile_index)) jp2_get_tile;

			l_codec->m_codec_data.m_decompression.opj_set_decoded_resolution_factor = (opj_bool (*) (void * p_codec,
																									OPJ_UINT32 res_factor,
																									opj_event_mgr_t * p_manager)) jp2_set_decoded_resolution_factor;

			l_codec->m_codec = jp2_create(OPJ_TRUE);

			if (! l_codec->m_codec) {
				opj_free(l_codec);
				return 00;
			}

			break;
		case CODEC_UNKNOWN:
		case CODEC_JPT:
		default:
			opj_free(l_codec);
			return 00;
	}

	set_default_event_handler(&(l_codec->m_event_mgr));
	return (opj_codec_t*) l_codec;
}

/* DEPRECATED */
void OPJ_CALLCONV opj_destroy_decompress(opj_dinfo_t *dinfo) {
	if(dinfo) {
		/* destroy the codec */
		switch(dinfo->codec_format) {
			case CODEC_J2K:
			case CODEC_JPT:
				j2k_destroy_decompress((opj_j2k_t*)dinfo->j2k_handle);
				break;
			case CODEC_JP2:
				jp2_destroy_decompress((opj_jp2_t*)dinfo->jp2_handle);
				break;
			case CODEC_UNKNOWN:
			default:
				break;
		}
		/* destroy the decompressor */
		opj_free(dinfo);
	}
}

void OPJ_CALLCONV opj_set_default_decoder_parameters(opj_dparameters_t *parameters) {
	if(parameters) {
		memset(parameters, 0, sizeof(opj_dparameters_t));
		/* default decoding parameters */
		parameters->cp_layer = 0;
		parameters->cp_reduce = 0;
		parameters->cp_limit_decoding = NO_LIMITATION;

		parameters->decod_format = -1;
		parameters->cod_format = -1;
		parameters->flags = 0;		
/* UniPG>> */
#ifdef USE_JPWL
		parameters->jpwl_correct = OPJ_FALSE;
		parameters->jpwl_exp_comps = JPWL_EXPECTED_COMPONENTS;
		parameters->jpwl_max_tiles = JPWL_MAXIMUM_TILES;
#endif /* USE_JPWL */
/* <<UniPG */
	}
}

/* DEPRECATED */
void OPJ_CALLCONV opj_setup_decoder(opj_dinfo_t *dinfo, opj_dparameters_t *parameters) {
	if(dinfo && parameters) {
		switch(dinfo->codec_format) {
			case CODEC_J2K:
			case CODEC_JPT:
				j2k_setup_decoder((opj_j2k_t*)dinfo->j2k_handle, parameters);
				break;
			case CODEC_JP2:
				jp2_setup_decoder((opj_jp2_t*)dinfo->jp2_handle, parameters);
				break;
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
}

opj_bool OPJ_CALLCONV opj_setup_decoder_v2(	opj_codec_t *p_codec, 
											opj_dparameters_t *parameters 
											)
{
	if (p_codec && parameters) { 
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;

		if (! l_codec->is_decompressor) {
			opj_event_msg_v2(&(l_codec->m_event_mgr), EVT_ERROR, "Codec provided to the opj_setup_decoder function is not a decompressor handler.\n");
			return OPJ_FALSE;
		}

		l_codec->m_codec_data.m_decompression.opj_setup_decoder(l_codec->m_codec,
																parameters);
		return OPJ_TRUE;
	}
	return OPJ_FALSE;
}

/* DEPRECATED */
opj_image_t* OPJ_CALLCONV opj_decode(opj_dinfo_t *dinfo, opj_cio_t *cio) {
	return opj_decode_with_info(dinfo, cio, NULL);
}

/* DEPRECATED */
opj_image_t* OPJ_CALLCONV opj_decode_with_info(opj_dinfo_t *dinfo, opj_cio_t *cio, opj_codestream_info_t *cstr_info) {
	if(dinfo && cio) {
		switch(dinfo->codec_format) {
			case CODEC_J2K:
				return j2k_decode((opj_j2k_t*)dinfo->j2k_handle, cio, cstr_info);
			case CODEC_JPT:
				return j2k_decode_jpt_stream((opj_j2k_t*)dinfo->j2k_handle, cio, cstr_info);
			case CODEC_JP2:
				return opj_jp2_decode((opj_jp2_t*)dinfo->jp2_handle, cio, cstr_info);
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
	return NULL;
}

/* DEPRECATED */
opj_cinfo_t* OPJ_CALLCONV opj_create_compress(OPJ_CODEC_FORMAT format) {
	opj_cinfo_t *cinfo = (opj_cinfo_t*)opj_calloc(1, sizeof(opj_cinfo_t));
	if(!cinfo) return NULL;
	cinfo->is_decompressor = OPJ_FALSE;
	switch(format) {
		case CODEC_J2K:
			/* get a J2K coder handle */
			cinfo->j2k_handle = (void*)j2k_create_compress((opj_common_ptr)cinfo);
			if(!cinfo->j2k_handle) {
				opj_free(cinfo);
				return NULL;
			}
			break;
		case CODEC_JP2:
			/* get a JP2 coder handle */
			cinfo->jp2_handle = (void*)jp2_create_compress((opj_common_ptr)cinfo);
			if(!cinfo->jp2_handle) {
				opj_free(cinfo);
				return NULL;
			}
			break;
		case CODEC_JPT:
		case CODEC_UNKNOWN:
		default:
			opj_free(cinfo);
			return NULL;
	}

	cinfo->codec_format = format;

	return cinfo;
}


opj_codec_t* OPJ_CALLCONV opj_create_compress_v2(OPJ_CODEC_FORMAT p_format)
{
	opj_codec_private_t *l_codec = 00;

	l_codec = (opj_codec_private_t*)opj_calloc(1, sizeof(opj_codec_private_t));
	if (!l_codec) {
		return 00;
	}
	memset(l_codec, 0, sizeof(opj_codec_private_t));
	
	l_codec->is_decompressor = 0;

	switch(p_format) {
		case CODEC_J2K:
			l_codec->m_codec_data.m_compression.opj_encode = (opj_bool (*) (void *,
																			struct opj_stream_private *,
																			struct opj_event_mgr * )) j2k_encode_v2;

			l_codec->m_codec_data.m_compression.opj_end_compress = (opj_bool (*) (	void *,
																					struct opj_stream_private *,
																					struct opj_event_mgr *)) j2k_end_compress;

			l_codec->m_codec_data.m_compression.opj_start_compress = (opj_bool (*) (void *,
																					struct opj_stream_private *,
																					struct opj_image * ,
																					struct opj_event_mgr *)) j2k_start_compress;

			l_codec->m_codec_data.m_compression.opj_write_tile = (opj_bool (*) (void *,
																				OPJ_UINT32,
																				OPJ_BYTE*,
																				OPJ_UINT32,
																				struct opj_stream_private *,
																				struct opj_event_mgr *) ) j2k_write_tile;

			l_codec->m_codec_data.m_compression.opj_destroy = (void (*) (void *)) j2k_destroy;

			l_codec->m_codec_data.m_compression.opj_setup_encoder = (void (*) (	void *,
																				opj_cparameters_t *,
																				struct opj_image *,
																				struct opj_event_mgr * )) j2k_setup_encoder_v2;

			l_codec->m_codec = j2k_create_compress_v2();
			if (! l_codec->m_codec) {
				opj_free(l_codec);
				return 00;
			}

			break;

		case CODEC_JP2:
			/* get a JP2 decoder handle */
			l_codec->m_codec_data.m_compression.opj_encode = (opj_bool (*) (void *,
																			struct opj_stream_private *,
																			struct opj_event_mgr * )) opj_jp2_encode_v2;

			l_codec->m_codec_data.m_compression.opj_end_compress = (opj_bool (*) (	void *,
																					struct opj_stream_private *,
																					struct opj_event_mgr *)) jp2_end_compress;

			l_codec->m_codec_data.m_compression.opj_start_compress = (opj_bool (*) (void *,
																					struct opj_stream_private *,
																					struct opj_image * ,
																					struct opj_event_mgr *))  jp2_start_compress;

			l_codec->m_codec_data.m_compression.opj_write_tile = (opj_bool (*) (void *,
																				OPJ_UINT32,
																				OPJ_BYTE*,
																				OPJ_UINT32,
																				struct opj_stream_private *,
																				struct opj_event_mgr *)) jp2_write_tile;

			l_codec->m_codec_data.m_compression.opj_destroy = (void (*) (void *)) jp2_destroy;

			l_codec->m_codec_data.m_compression.opj_setup_encoder = (void (*) (	void *,
																				opj_cparameters_t *,
																				struct opj_image *,
																				struct opj_event_mgr * )) jp2_setup_encoder;

			l_codec->m_codec = jp2_create(OPJ_FALSE);
			if (! l_codec->m_codec) {
				opj_free(l_codec);
				return 00;
			}

			break;

		case CODEC_UNKNOWN:
		case CODEC_JPT:
		default:
			opj_free(l_codec);
			return 00;
	}

	set_default_event_handler(&(l_codec->m_event_mgr));
	return (opj_codec_t*) l_codec;
}

/* DEPRECATED */
void OPJ_CALLCONV opj_destroy_compress(opj_cinfo_t *cinfo) {
	if(cinfo) {
		/* destroy the codec */
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				j2k_destroy_compress((opj_j2k_t*)cinfo->j2k_handle);
				break;
			case CODEC_JP2:
				jp2_destroy_compress((opj_jp2_t*)cinfo->jp2_handle);
				break;
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
		/* destroy the decompressor */
		opj_free(cinfo);
	}
}

void OPJ_CALLCONV opj_set_default_encoder_parameters(opj_cparameters_t *parameters) {
	if(parameters) {
		memset(parameters, 0, sizeof(opj_cparameters_t));
		/* default coding parameters */
		parameters->cp_cinema = OFF; 
		parameters->max_comp_size = 0;
		parameters->numresolution = 6;
		parameters->cp_rsiz = STD_RSIZ;
		parameters->cblockw_init = 64;
		parameters->cblockh_init = 64;
		parameters->prog_order = LRCP;
		parameters->roi_compno = -1;		/* no ROI */
		parameters->subsampling_dx = 1;
		parameters->subsampling_dy = 1;
		parameters->tp_on = 0;
		parameters->decod_format = -1;
		parameters->cod_format = -1;
		parameters->tcp_rates[0] = 0;   
		parameters->tcp_numlayers = 0;
		parameters->cp_disto_alloc = 0;
		parameters->cp_fixed_alloc = 0;
		parameters->cp_fixed_quality = 0;
		parameters->jpip_on = OPJ_FALSE;
/* UniPG>> */
#ifdef USE_JPWL
		parameters->jpwl_epc_on = OPJ_FALSE;
		parameters->jpwl_hprot_MH = -1; /* -1 means unassigned */
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				parameters->jpwl_hprot_TPH_tileno[i] = -1; /* unassigned */
				parameters->jpwl_hprot_TPH[i] = 0; /* absent */
			}
		};
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_PACKSPECS; i++) {
				parameters->jpwl_pprot_tileno[i] = -1; /* unassigned */
				parameters->jpwl_pprot_packno[i] = -1; /* unassigned */
				parameters->jpwl_pprot[i] = 0; /* absent */
			}
		};
		parameters->jpwl_sens_size = 0; /* 0 means no ESD */
		parameters->jpwl_sens_addr = 0; /* 0 means auto */
		parameters->jpwl_sens_range = 0; /* 0 means packet */
		parameters->jpwl_sens_MH = -1; /* -1 means unassigned */
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				parameters->jpwl_sens_TPH_tileno[i] = -1; /* unassigned */
				parameters->jpwl_sens_TPH[i] = -1; /* absent */
			}
		};
#endif /* USE_JPWL */
/* <<UniPG */
	}
}

/**
 * Helper function.
 * Sets the stream to be a file stream. The FILE must have been open previously.
 * @param		p_stream	the stream to modify
 * @param		p_file		handler to an already open file.
*/
opj_stream_t* OPJ_CALLCONV opj_stream_create_default_file_stream (FILE * p_file, opj_bool p_is_read_stream)
{
	return opj_stream_create_file_stream(p_file,J2K_STREAM_CHUNK_SIZE,p_is_read_stream);
}

opj_stream_t* OPJ_CALLCONV opj_stream_create_file_stream (	FILE * p_file, 
															OPJ_SIZE_T p_size, 
															opj_bool p_is_read_stream)
{
	opj_stream_t* l_stream = 00;

	if (! p_file) {
		return NULL;
	}

	l_stream = opj_stream_create(p_size,p_is_read_stream);
	if (! l_stream) {
		return NULL;
	}

	opj_stream_set_user_data(l_stream, p_file);
	opj_stream_set_user_data_length(l_stream, opj_get_data_length_from_file(p_file));
	opj_stream_set_read_function(l_stream, (opj_stream_read_fn) opj_read_from_file);
	opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_file);
	opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_file);
	opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_file);

	return l_stream;
}
/* DEPRECATED */
void OPJ_CALLCONV opj_setup_encoder(opj_cinfo_t *cinfo, opj_cparameters_t *parameters, opj_image_t *image) {
	if(cinfo && parameters && image) {
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				j2k_setup_encoder((opj_j2k_t*)cinfo->j2k_handle, parameters, image);
				break;
			case CODEC_JP2:
				jp2_setup_encoder((opj_jp2_v2_t*)cinfo->jp2_handle, parameters, image, NULL);
				break;
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
}

opj_bool OPJ_CALLCONV opj_setup_encoder_v2(	opj_codec_t *p_codec, 
											opj_cparameters_t *parameters, 
											opj_image_t *p_image)
{
	if (p_codec && parameters && p_image) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;

		if (! l_codec->is_decompressor) {
			l_codec->m_codec_data.m_compression.opj_setup_encoder(	l_codec->m_codec,
																	parameters,
																	p_image,
																	&(l_codec->m_event_mgr) );
			return OPJ_TRUE;
		}
	}

	return OPJ_FALSE;
}

opj_bool OPJ_CALLCONV opj_start_compress (	opj_codec_t *p_codec,
											opj_image_t * p_image,
											opj_stream_t *p_stream)
{
	if (p_codec && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return l_codec->m_codec_data.m_compression.opj_start_compress(	l_codec->m_codec,
																			l_stream,
																			p_image,
																			&(l_codec->m_event_mgr));
		}
	}

	return OPJ_FALSE;
}

opj_bool OPJ_CALLCONV opj_encode_v2(opj_codec_t *p_info, opj_stream_t *p_stream)
{
	if (p_info && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_info;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			l_codec->m_codec_data.m_compression.opj_encode(	l_codec->m_codec,
															l_stream,
															&(l_codec->m_event_mgr));
			return OPJ_TRUE;
		}
	}

	return OPJ_FALSE;

}

opj_bool OPJ_CALLCONV opj_end_compress (opj_codec_t *p_codec,
										opj_stream_t *p_stream)
{
	if (p_codec && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return l_codec->m_codec_data.m_compression.opj_end_compress(l_codec->m_codec,
																		l_stream,
																		&(l_codec->m_event_mgr));
		}
	}
	return OPJ_FALSE;

}

/* DEPRECATED */
opj_bool OPJ_CALLCONV opj_encode(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, char *index) {
	if (index != NULL)
		opj_event_msg((opj_common_ptr)cinfo, EVT_WARNING, "Set index to NULL when calling the opj_encode function.\n"
		"To extract the index, use the opj_encode_with_info() function.\n"
		"No index will be generated during this encoding\n");
	return opj_encode_with_info(cinfo, cio, image, NULL);
}

/* DEPRECATED */
opj_bool OPJ_CALLCONV opj_encode_with_info(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info) {
	if(cinfo && cio && image) {
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				return j2k_encode((opj_j2k_t*)cinfo->j2k_handle, cio, image, cstr_info);
			case CODEC_JP2:
				return opj_jp2_encode((opj_jp2_t*)cinfo->jp2_handle, cio, image, cstr_info);	    
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
	return OPJ_FALSE;
}

/* DEPRECATED */
void OPJ_CALLCONV opj_destroy_cstr_info(opj_codestream_info_t *cstr_info) {
	if (cstr_info) {
		int tileno;
		for (tileno = 0; tileno < cstr_info->tw * cstr_info->th; tileno++) {
			opj_tile_info_t *tile_info = &cstr_info->tile[tileno];
			opj_free(tile_info->thresh);
			opj_free(tile_info->packet);
			opj_free(tile_info->tp);
			opj_free(tile_info->marker);
		}
		opj_free(cstr_info->tile);
		opj_free(cstr_info->marker);
		opj_free(cstr_info->numdecompos);
	}
}


opj_bool OPJ_CALLCONV opj_read_header (	opj_stream_t *p_stream,
										opj_codec_t *p_codec,
										opj_image_t **p_image )
{
	if (p_codec && p_stream) {
		opj_codec_private_t* l_codec = (opj_codec_private_t*) p_codec;
		opj_stream_private_t* l_stream = (opj_stream_private_t*) p_stream;

		if(! l_codec->is_decompressor) {
			opj_event_msg_v2(&(l_codec->m_event_mgr), EVT_ERROR, "Codec provided to the opj_read_header function is not a decompressor handler.\n");
			return OPJ_FALSE;
		}

		return l_codec->m_codec_data.m_decompression.opj_read_header(	l_stream,
																		l_codec->m_codec,
																		p_image,
																		&(l_codec->m_event_mgr) );
	}

	return OPJ_FALSE;
}


void OPJ_CALLCONV opj_destroy_codec(opj_codec_t *p_codec)
{
	if (p_codec) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;

		if (l_codec->is_decompressor) {
			l_codec->m_codec_data.m_decompression.opj_destroy(l_codec->m_codec);
		}
		else {
			l_codec->m_codec_data.m_compression.opj_destroy(l_codec->m_codec);
		}

		l_codec->m_codec = 00;
		opj_free(l_codec);
	}
}

/**
 * Sets the given area to be decoded. This function should be called right after opj_read_header and before any tile header reading.
 *
 * @param	p_codec			the jpeg2000 codec.
 * @param	p_start_x		the left position of the rectangle to decode (in image coordinates).
 * @param	p_end_x			the right position of the rectangle to decode (in image coordinates).
 * @param	p_start_y		the up position of the rectangle to decode (in image coordinates).
 * @param	p_end_y			the bottom position of the rectangle to decode (in image coordinates).
 *
 * @return	true			if the area could be set.
 */
opj_bool OPJ_CALLCONV opj_set_decode_area(	opj_codec_t *p_codec,
											opj_image_t* p_image,
											OPJ_INT32 p_start_x, OPJ_INT32 p_start_y,
											OPJ_INT32 p_end_x, OPJ_INT32 p_end_y
											)
{
	if (p_codec) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		
		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}

		return  l_codec->m_codec_data.m_decompression.opj_set_decode_area(	l_codec->m_codec,
																			p_image,
																			p_start_x, p_start_y,
																			p_end_x, p_end_y,
																			&(l_codec->m_event_mgr) );
	}
	return OPJ_FALSE;
}

/**
 * Reads a tile header. This function is compulsory and allows one to know the size of the tile thta will be decoded.
 * The user may need to refer to the image got by opj_read_header to understand the size being taken by the tile.
 *
 * @param	p_codec			the jpeg2000 codec.
 * @param	p_tile_index	pointer to a value that will hold the index of the tile being decoded, in case of success.
 * @param	p_data_size		pointer to a value that will hold the maximum size of the decoded data, in case of success. In case
 *							of truncated codestreams, the actual number of bytes decoded may be lower. The computation of the size is the same
 *							as depicted in opj_write_tile.
 * @param	p_tile_x0		pointer to a value that will hold the x0 pos of the tile (in the image).
 * @param	p_tile_y0		pointer to a value that will hold the y0 pos of the tile (in the image).
 * @param	p_tile_x1		pointer to a value that will hold the x1 pos of the tile (in the image).
 * @param	p_tile_y1		pointer to a value that will hold the y1 pos of the tile (in the image).
 * @param	p_nb_comps		pointer to a value that will hold the number of components in the tile.
 * @param	p_should_go_on	pointer to a boolean that will hold the fact that the decoding should go on. In case the
 *							codestream is over at the time of the call, the value will be set to false. The user should then stop
 *							the decoding.
 * @param	p_stream		the stream to decode.
 * @return	true			if the tile header could be decoded. In case the decoding should end, the returned value is still true.
 *							returning false may be the result of a shortage of memory or an internal error.
 */
opj_bool OPJ_CALLCONV opj_read_tile_header(	opj_codec_t *p_codec,
											opj_stream_t * p_stream,
											OPJ_UINT32 * p_tile_index,
											OPJ_UINT32 * p_data_size,
											OPJ_INT32 * p_tile_x0, OPJ_INT32 * p_tile_y0,
											OPJ_INT32 * p_tile_x1, OPJ_INT32 * p_tile_y1,
											OPJ_UINT32 * p_nb_comps,
											opj_bool * p_should_go_on)
{
	if (p_codec && p_stream && p_data_size && p_tile_index) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}

		return l_codec->m_codec_data.m_decompression.opj_read_tile_header(	l_codec->m_codec,
																			p_tile_index,
																			p_data_size,
																			p_tile_x0, p_tile_y0,
																			p_tile_x1, p_tile_y1,
																			p_nb_comps,
																			p_should_go_on,
																			l_stream,
																			&(l_codec->m_event_mgr));
	}
	return OPJ_FALSE;
}

/**
 * Reads a tile data. This function is compulsory and allows one to decode tile data. opj_read_tile_header should be called before.
 * The user may need to refer to the image got by opj_read_header to understand the size being taken by the tile.
 *
 * @param	p_codec			the jpeg2000 codec.
 * @param	p_tile_index	the index of the tile being decoded, this should be the value set by opj_read_tile_header.
 * @param	p_data			pointer to a memory block that will hold the decoded data.
 * @param	p_data_size		size of p_data. p_data_size should be bigger or equal to the value set by opj_read_tile_header.
 * @param	p_stream		the stream to decode.
 *
 * @return	true			if the data could be decoded.
 */
opj_bool OPJ_CALLCONV opj_decode_tile_data(	opj_codec_t *p_codec,
											OPJ_UINT32 p_tile_index,
											OPJ_BYTE * p_data,
											OPJ_UINT32 p_data_size,
											opj_stream_t *p_stream
											)
{
	if (p_codec && p_data && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}

		return l_codec->m_codec_data.m_decompression.opj_decode_tile_data(	l_codec->m_codec,
																			p_tile_index,
																			p_data,
																			p_data_size,
																			l_stream,
																			&(l_codec->m_event_mgr) );
	}
	return OPJ_FALSE;
}

/*
 *
 *
 */
void OPJ_CALLCONV opj_dump_codec(	opj_codec_t *p_codec,
									OPJ_INT32 info_flag,
									FILE* output_stream)
{
	if (p_codec) {
		opj_codec_private_t* l_codec = (opj_codec_private_t*) p_codec;

		l_codec->opj_dump_codec(l_codec->m_codec, info_flag, output_stream);
		return;
	}

	fprintf(stderr, "[ERROR] Input parameter of the dump_codec function are incorrect.\n");
	return;
}

/*
 *
 *
 */
opj_codestream_info_v2_t* OPJ_CALLCONV opj_get_cstr_info(opj_codec_t *p_codec)
{
	if (p_codec) {
		opj_codec_private_t* l_codec = (opj_codec_private_t*) p_codec;

		return l_codec->opj_get_codec_info(l_codec->m_codec);
	}

	return NULL;
}

/*
 *
 *
 */
void OPJ_CALLCONV opj_destroy_cstr_info_v2(opj_codestream_info_v2_t **cstr_info) {
	if (cstr_info) {

		if ((*cstr_info)->m_default_tile_info.tccp_info){
			opj_free((*cstr_info)->m_default_tile_info.tccp_info);
		}

		if ((*cstr_info)->tile_info){
			/* FIXME not used for the moment*/
		}

		opj_free((*cstr_info));
		(*cstr_info) = NULL;
	}
}

/*
 *
 *
 */
opj_codestream_index_t * OPJ_CALLCONV opj_get_cstr_index(opj_codec_t *p_codec)
{
	if (p_codec) {
		opj_codec_private_t* l_codec = (opj_codec_private_t*) p_codec;

		return l_codec->opj_get_codec_index(l_codec->m_codec);
	}

	return NULL;
}

/*
 *
 *
 */
void OPJ_CALLCONV opj_destroy_cstr_index(opj_codestream_index_t **p_cstr_index)
{
	if (*p_cstr_index){
		j2k_destroy_cstr_index(*p_cstr_index);
		(*p_cstr_index) = NULL;
	}
}

/*
 *
 *
 */
opj_bool OPJ_CALLCONV opj_decode_v2(opj_codec_t *p_codec,
									opj_stream_t *p_stream,
									opj_image_t* p_image)
{
	if (p_codec && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}

		return l_codec->m_codec_data.m_decompression.opj_decode(l_codec->m_codec,
																l_stream,
																p_image,
																&(l_codec->m_event_mgr) );
	}

	return OPJ_FALSE;
}

/*
 *
 *
 */
opj_bool OPJ_CALLCONV opj_end_decompress (	opj_codec_t *p_codec,
											opj_stream_t *p_stream)
{
	if (p_codec && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}
		
		return l_codec->m_codec_data.m_decompression.opj_end_decompress(l_codec->m_codec,
																		l_stream,
																		&(l_codec->m_event_mgr) );
	}

	return OPJ_FALSE;
}

/*
 *
 *
 */
opj_bool OPJ_CALLCONV opj_get_decoded_tile(	opj_codec_t *p_codec,
											opj_stream_t *p_stream,
											opj_image_t *p_image,
											OPJ_UINT32 tile_index)
{
	if (p_codec && p_stream) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (! l_codec->is_decompressor) {
			return OPJ_FALSE;
		}
		
		return l_codec->m_codec_data.m_decompression.opj_get_decoded_tile(	l_codec->m_codec,
																			l_stream,
																			p_image,
																			&(l_codec->m_event_mgr),
																			tile_index);
	}

	return OPJ_FALSE;
}

/*
 *
 *
 */
opj_bool OPJ_CALLCONV opj_set_decoded_resolution_factor(opj_codec_t *p_codec, 
														OPJ_UINT32 res_factor )
{
	opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;

	if ( !l_codec ){
		fprintf(stderr, "[ERROR] Input parameters of the setup_decoder function are incorrect.\n");
		return OPJ_FALSE;
	}

	l_codec->m_codec_data.m_decompression.opj_set_decoded_resolution_factor(l_codec->m_codec, 
																			res_factor,
																			&(l_codec->m_event_mgr) );
	return OPJ_TRUE;
}


opj_bool OPJ_CALLCONV opj_set_MCT(opj_cparameters_t *parameters,OPJ_FLOAT32 * pEncodingMatrix,OPJ_INT32 * p_dc_shift,OPJ_UINT32 pNbComp)
{
	OPJ_UINT32 l_matrix_size = pNbComp * pNbComp * sizeof(OPJ_FLOAT32);
	OPJ_UINT32 l_dc_shift_size = pNbComp * sizeof(OPJ_INT32);
	OPJ_UINT32 l_mct_total_size = l_matrix_size + l_dc_shift_size;

	/* add MCT capability */
	int rsiz = (int)parameters->cp_rsiz | (int)MCT;
	parameters->cp_rsiz = (OPJ_RSIZ_CAPABILITIES)rsiz;
	parameters->irreversible = 1;

	/* use array based MCT */
	parameters->tcp_mct = 2;
	parameters->mct_data = opj_malloc(l_mct_total_size);
	if (! parameters->mct_data) {
		return OPJ_FALSE;
	}

	memcpy(parameters->mct_data,pEncodingMatrix,l_matrix_size);
	memcpy(((OPJ_BYTE *) parameters->mct_data) +  l_matrix_size,p_dc_shift,l_dc_shift_size);

	return OPJ_TRUE;
}

/**
 * Writes a tile with the given data.
 *
 * @param	p_compressor		the jpeg2000 codec.
 * @param	p_tile_index		the index of the tile to write. At the moment, the tiles must be written from 0 to n-1 in sequence.
 * @param	p_data				pointer to the data to write. Data is arranged in sequence, data_comp0, then data_comp1, then ... NO INTERLEAVING should be set.
 * @param	p_data_size			this value os used to make sure the data being written is correct. The size must be equal to the sum for each component of tile_width * tile_height * component_size. component_size can be 1,2 or 4 bytes,
 *								depending on the precision of the given component.
 * @param	p_stream			the stream to write data to.
 */
opj_bool OPJ_CALLCONV opj_write_tile (	opj_codec_t *p_codec,
										OPJ_UINT32 p_tile_index,
										OPJ_BYTE * p_data,
										OPJ_UINT32 p_data_size,
										opj_stream_t *p_stream )
{
	if (p_codec && p_stream && p_data) {
		opj_codec_private_t * l_codec = (opj_codec_private_t *) p_codec;
		opj_stream_private_t * l_stream = (opj_stream_private_t *) p_stream;

		if (l_codec->is_decompressor) {
			return OPJ_FALSE;
		}

		return l_codec->m_codec_data.m_compression.opj_write_tile(	l_codec->m_codec,
																	p_tile_index,
																	p_data,
																	p_data_size,
																	l_stream,
																	&(l_codec->m_event_mgr) );
	}

	return OPJ_FALSE;
}
