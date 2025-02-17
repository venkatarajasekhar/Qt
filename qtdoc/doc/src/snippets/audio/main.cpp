/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioInput>

class Window2 : public QWidget
{
    Q_OBJECT

public slots:
//![0]
    void stateChanged(QAudio::State newState)
    {
        switch(newState) {
            case QAudio::StopState:
            if (input->error() != QAudio::NoError) {
                // Error handling
            } else {

            }
            break;
//![0]
        default:
            ;
        }
    }

private:
    QAudioInput *input;

};

class Window : public QWidget
{
    Q_OBJECT

public:
    Window()
    {
        output = new QAudioOutput;
        connect(output, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(stateChanged(QAudio::State)));
    }

private:
    void setupFormat()
    {
//![1]
        QAudioFormat format;
        format.setFrequency(44100);
//![1]
        format.setChannels(2);
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
//![2]
        format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

        if (!info.isFormatSupported(format))
            format = info.nearestFormat(format);
//![2]
    }

public slots:
//![3]
    void stateChanged(QAudio::State newState)
    {
        switch (newState) {
            case QAudio::StopState:
                if (output->error() != QAudio::NoError) {
                    // Perform error handling
                } else {
                    // Normal stop
                }
                break;
//![3]

            // Handle
            case QAudio::ActiveState:
                // Handle active state...
                break;
            break;
        default:
            ;
        }
    }

private:
    QAudioOutput *output;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Window window;
    window.show();

    return app.exec();
}


#include "main.moc"

