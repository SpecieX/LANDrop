/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, yvbbrjdr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QMessageBox>

#include "filetransferdialog.h"
#include "ui_filetransferdialog.h"

FileTransferDialog::FileTransferDialog(QWidget *parent, FileTransferSession::TransferDirection dir,
                                       QTcpSocket *socket, const QList<QSharedPointer<QFile>> &files) :
    QDialog(parent), ui(new Ui::FileTransferDialog), session(nullptr, dir, socket, files)
{
    ui->setupUi(this);

    connect(&session, &FileTransferSession::printMessage, ui->statusLabel, &QLabel::setText);
    connect(&session, &FileTransferSession::updateProgress, this, &FileTransferDialog::sessionUpdateProgress);
    connect(&session, &FileTransferSession::errorOccurred, this, &FileTransferDialog::sessionErrorOccurred);
    connect(&session, &FileTransferSession::fileMetadataReady, this, &FileTransferDialog::sessionFileMetadataReady);
    connect(&session, &FileTransferSession::ended, this, &FileTransferDialog::accept);
    session.start();
}

FileTransferDialog::~FileTransferDialog()
{
    delete ui;
}

void FileTransferDialog::sessionUpdateProgress(double progress)
{
    ui->progressBar->setValue(ui->progressBar->maximum() * progress);
}

void FileTransferDialog::sessionErrorOccurred(const QString &msg)
{
    QMessageBox::critical(this, QApplication::applicationName(), msg);
    done(Rejected);
}

void FileTransferDialog::sessionFileMetadataReady(const QList<FileTransferSession::FileMetadata> &metadata,
                                                  quint64 totalSize,
                                                  const QString &machineName,
                                                  const QString &sessionKeyDigest)
{
    show();

    QString totalSizeStr = locale().formattedDataSize(totalSize, 2, QLocale::DataSizeTraditionalFormat);
    QString msg;
    if (metadata.size() == 1) {
        msg = tr("%1 would like to share a file \"%2\" of size %3.")
                .arg(machineName, metadata.first().filename, totalSizeStr);
    } else {
        msg = tr("%1 would like to share %2 files of total size %3.")
                .arg(machineName).arg(metadata.size()).arg(totalSizeStr);
    }
    msg += tr("\nConfirm that the code \"%1\" is shown on the sending device.").arg(sessionKeyDigest);
    msg += tr("\nWould you like to receive it?");

    bool result = QMessageBox::question(this, QApplication::applicationName(), msg,
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::Yes) == QMessageBox::Yes;
    session.respond(result);
    if (!result)
        hide();
}
