/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mfdecodersourcereader.h"

MFDecoderSourceReader::MFDecoderSourceReader(QObject *parent)
    : m_cRef(1)
    , m_sourceReader(0)
    , m_source(0)
{
    Q_UNUSED(parent)
}

void MFDecoderSourceReader::shutdown()
{
    if (m_source) {
        m_source->Release();
        m_source = NULL;
    }
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = NULL;
    }
}

IMFMediaSource* MFDecoderSourceReader::mediaSource()
{
    return m_source;
}

IMFMediaType* MFDecoderSourceReader::setSource(IMFMediaSource *source, const QAudioFormat &audioFormat)
{
    IMFMediaType *mediaType = NULL;
    if (m_source == source)
        return mediaType;
    if (m_source) {
        m_source->Release();
        m_source = NULL;
    }
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = NULL;
    }
    if (!source)
        return mediaType;
    IMFAttributes *attr = NULL;
    MFCreateAttributes(&attr, 1);
    if (SUCCEEDED(attr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this))) {
        if (SUCCEEDED(MFCreateSourceReaderFromMediaSource(source, attr, &m_sourceReader))) {
            m_source = source;
            m_source->AddRef();
            m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE);
            m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
            IMFMediaType *pPartialType = NULL;
            MFCreateMediaType(&pPartialType);
            pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);

            if (audioFormat.sampleType() == QAudioFormat::Float) {
                pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            } else {
                pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
            }

            m_sourceReader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), NULL, pPartialType);
            pPartialType->Release();
            m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &mediaType);
            // Ensure the stream is selected.
            m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
        }
        attr->Release();
    }
    return mediaType;
}

void MFDecoderSourceReader::reset()
{
    if (!m_sourceReader)
        return;
    PROPVARIANT vPos;
    PropVariantInit(&vPos);
    vPos.vt = VT_I8;
    vPos.uhVal.QuadPart = 0;
    m_sourceReader->SetCurrentPosition(GUID_NULL, vPos);
}

void MFDecoderSourceReader::readNextSample()
{
    if (!m_sourceReader)
        return;
    m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, NULL, NULL, NULL);
}

QList<IMFSample*> MFDecoderSourceReader::takeSamples() //internal samples will be cleared after this
{
    QList<IMFSample*> samples;
    m_samplesMutex.lock();
    samples = m_cachedSamples;
    m_cachedSamples.clear();
    m_samplesMutex.unlock();
    return samples;
}

//from IUnknown
STDMETHODIMP MFDecoderSourceReader::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFSourceReaderCallback) {
        *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) MFDecoderSourceReader::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MFDecoderSourceReader::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        this->deleteLater();
    }
    return cRef;
}

//from IMFSourceReaderCallback
STDMETHODIMP MFDecoderSourceReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
    DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
{
    Q_UNUSED(hrStatus);
    Q_UNUSED(dwStreamIndex);
    Q_UNUSED(llTimestamp);
    if (pSample) {
        pSample->AddRef();
        m_samplesMutex.lock();
        m_cachedSamples.push_back(pSample);
        m_samplesMutex.unlock();
        emit sampleAdded();
    } else if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM) {
        emit finished();
    }
    return S_OK;
}

STDMETHODIMP MFDecoderSourceReader::OnFlush(DWORD)
{
    return S_OK;
}

STDMETHODIMP MFDecoderSourceReader::OnEvent(DWORD, IMFMediaEvent*)
{
    return S_OK;
}
