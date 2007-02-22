/*
 * Copyright (c) 2007, Digital Signal Processing Laboratory, UniversitÓ degli studi di Perugia (UPG), Italy
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
/////////////////////////////////////////////////////////////////////////////
// Name:        imagj2k.h
// Purpose:     wxImage JPEG 2000 raw codestream handler
// Author:      G. Baruffa - based on imagjpeg.h, Vaclav Slavik
// RCS-ID:      $Id: imagj2k.h,v 0.0 2007/02/08 23:45:00 VZ Exp $
// Copyright:   (c) Giuseppe Baruffa
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGJ2K_H_
#define _WX_IMAGJ2K_H_

#include "wx/defs.h"

//-----------------------------------------------------------------------------
// wxJ2KHandler
//-----------------------------------------------------------------------------

#if wxUSE_LIBOPENJPEG

#include "wx/image.h"

#define wxBITMAP_TYPE_J2K	47

class WXDLLEXPORT wxJ2KHandler: public wxImageHandler
{
public:
    inline wxJ2KHandler()
    {
        m_name = wxT("JPEG 2000 codestream file");
        m_extension = wxT("j2k");
        m_type = wxBITMAP_TYPE_J2K;
        m_mime = wxT("image/j2k");
    }

#if wxUSE_STREAMS
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );
    virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=true );
protected:
    virtual bool DoCanRead( wxInputStream& stream );
#endif

private:
    DECLARE_DYNAMIC_CLASS(wxJ2KHandler)
};

#endif // wxUSE_LIBOPENJPEG

#endif // _WX_IMAGJ2K_H_

