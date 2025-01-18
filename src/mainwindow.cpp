#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef _WIN64
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
#ifdef _WIN64
    qDebug() << "Program successfully started on Windows!";
#else
    qDebug() << "Program successfully started on Unix type system!";
#endif
    QUrl imageUrl("http://qt.digia.com/Documents/1/QtLogo.png");
    connect(ui->b_download, &QPushButton::pressed, this, &MainWindow::on_b_download_pressed);
    connect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_b_download_pressed() {
    disconnect(ui->b_download, &QPushButton::pressed, this, &MainWindow::on_b_download_pressed);
    QProcess *downloadProcess = new QProcess(this);
    downloadProcess->setProgram("yt-dlp");

    QString videoUrl = ui->e_inputUrl->text();
    QString savePath = QFileDialog::getSaveFileName(this, "Save Video", videoTitle, "All Files (*)");

    QString selectedFormat = ui->comboBoxFormats->currentText();
    QString formatId = extractFormatId(selectedFormat);

    if (formatId.isEmpty()) {
        qDebug() << "No valid format selected!";
        return;
    }

    if (savePath.isEmpty()) {
        savePath = "video";
    }

    if (!savePath.contains(".")) {
        if (selectedFormat.contains("mp4")) {
            savePath += ".mp4";
        } else if (selectedFormat.contains("webm")) {
            savePath += ".webm";
        } else {
            savePath += ".mp4";
        }
    }

    QStringList arguments;
    arguments << "-f"
              << formatId
              << "--output"
              << savePath
              << videoUrl
              << "--quiet";

    qDebug() << "Arguments:" << arguments;

    downloadProcess->setArguments(arguments);

    ui->progressSlider->setRange(0, 100);
    ui->progressSlider->setValue(0);

    connect(downloadProcess, &QProcess::readyReadStandardOutput, this, [this, downloadProcess]() {
        QByteArray output = downloadProcess->readAllStandardOutput();
        QString outputStr(output);

        if (outputStr.contains("%")) {
            QStringList parts = outputStr.split(" ");
            for (const QString &part : parts) {
                if (part.endsWith("%")) {
                    bool ok;
                    int progress = part.left(part.length() - 1).toInt(&ok);
                    if (ok) {
                        ui->progressSlider->setValue(progress);
                        qDebug() << "Download progress:" << progress << "%";
                    }
                }
            }
        }
    });

    connect(downloadProcess, &QProcess::finished, this, [this, downloadProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed!";
        } else {
            qDebug() << "Download finished with exit code:" << exitCode;
        }

        QByteArray errorOutput = downloadProcess->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            qDebug() << "Error output:" << errorOutput;
        }

        downloadProcess->deleteLater();
    });

    downloadProcess->start();
}
void MainWindow::on_searchButton_pressed() {
    disconnect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);

    QString videoUrl = ui->e_inputUrl->text();
    QString urlType = getUrlType(videoUrl);

    qDebug() << "URL type = "<< urlType;

    if (urlType == "video") {
        fetchVideoDetails(videoUrl);
        fetchAvailableFormats(videoUrl);
    } else if (urlType == "playlist") {
        qDebug() << "Playlist detected!";
    } else if (urlType == "channel") {
        qDebug() << "Channel detected!";
    } else {
        qDebug() << "Unknown URL type!";
    }

    connect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);
}

void MainWindow::fetchAvailableFormats(const QString &videoUrl) {
    QProcess *formatProcess = new QProcess(this);
    formatProcess->setProgram("yt-dlp");
    formatProcess->setArguments(QStringList() << "-F" << videoUrl);

    connect(formatProcess, &QProcess::finished, this, [this, formatProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed while fetching formats!";
            formatProcess->deleteLater();
            return;
        }

        QString output = formatProcess->readAllStandardOutput().trimmed();
        QStringList outputLines = output.split("\n");

        QStringList formats;
        QMap<QString, QString> videoFormats;
        QMap<QString, QString> audioFormats;
        formatMap.clear();

        for (const QString &line : outputLines) {
            if (line.contains("audio") || line.contains("video")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));

                if (parts.size() > 4) {
                    QString formatId = parts[0];
                    QString resolution = parts[1];
                    QString codec = parts[2];
                    QString type;

                    if (line.contains("video")) {
                        type = "video";
                    } else if (line.contains("audio")) {
                        type = "audio";
                    } else {
                        continue;
                    }

                    if (parts.last().contains("webm_dash") || parts.last().contains("mp4_dash") || parts.last().contains("m4a_dash")) {
                        continue;
                    }

                    QString readableFormat;
                    if (type == "video") {
                        readableFormat = resolution + " (" + codec + ")";
                        videoFormats[readableFormat] = formatId;
                    } else if (type == "audio") {
                        readableFormat = codec;
                        audioFormats[readableFormat] = formatId;
                    }
                }
            }
        }

        QStringList videoKeys = videoFormats.keys();
        QStringList audioKeys = audioFormats.keys();
        std::sort(videoKeys.begin(), videoKeys.end());
        std::sort(audioKeys.begin(), audioKeys.end());

        if (!videoKeys.isEmpty() && !audioKeys.isEmpty()) {
            ui->comboBoxFormats->clear();

            for (const QString &videoKey : videoKeys) {
                formats.append(videoKey + " (video only)");
                formatMap[videoKey + " (video only)"] = videoFormats[videoKey];
            }

            for (const QString &audioKey : audioKeys) {
                formats.append(audioKey + " (audio only)");
                formatMap[audioKey + " (audio only)"] = audioFormats[audioKey];
            }

            for (const QString &videoKey : videoKeys) {
                for (const QString &audioKey : audioKeys) {
                    QString combinedFormat = videoKey + " + " + audioKey;

                    combinedFormat.replace("(audio only)", "").replace("(audio)", "").trimmed();
                    formats.append(combinedFormat);

                    formatMap[combinedFormat] = videoFormats[videoKey] + "+" + audioFormats[audioKey];
                }
            }

            ui->comboBoxFormats->addItems(formats); // Заполняем ComboBox
        } else {
            qDebug() << "No valid video or audio formats found for this video!";
        }

        formatProcess->deleteLater();
    });

    formatProcess->start();
}




void MainWindow::downloadAndCombine(const QString &videoUrl, const QString &videoId, const QString &audioId) {
    QProcess *process = new QProcess(this);
    process->setProgram("yt-dlp");
    process->setArguments(QStringList() << "-f"
                                        << videoId + "+" + audioId
                                        << "-o"
                                        << videoUrl);

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed while downloading and combining!";
            process->deleteLater();
            return;
        }

        qDebug() << "Video and audio successfully downloaded and combined!";
        process->deleteLater();
    });

    process->start();
}

void MainWindow::fetchVideoDetails(const QString &videoUrl) {
    QProcess *thumbnailProcess = new QProcess(this);
    thumbnailProcess->setProgram("yt-dlp");
    thumbnailProcess->setArguments(QStringList() << "--quiet"
                                                 << "--list-thumbnails"
                                                 << videoUrl);

    connect(thumbnailProcess, &QProcess::finished, this, [this, thumbnailProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed while fetching thumbnails!";
            thumbnailProcess->deleteLater();
            return;
        }

        QString output = thumbnailProcess->readAllStandardOutput().trimmed();
        QStringList outputLines = output.split("\n");

        QString thumbnailUrl;
        for (const QString &line : outputLines) {
            if (line.contains("maxresdefault.jpg")) {
                QStringList parts = line.split(" ");
                thumbnailUrl = parts.last().trimmed();
                break;
            }
        }

        if (!thumbnailUrl.isEmpty()) {
            qDebug() << "Found maxresdefault thumbnail URL:" << thumbnailUrl;
            QUrl imageUrl(thumbnailUrl);
            loadImage(imageUrl);
        } else {
            qDebug() << "maxresdefault thumbnail not found!";
        }

        thumbnailProcess->deleteLater();
    });

    thumbnailProcess->start();

    QProcess *titleProcess = new QProcess(this);
    titleProcess->setProgram("yt-dlp");
    titleProcess->setArguments(QStringList() << "--quiet"
                                             << "--print"
                                             << "title"
                                             << "--encoding" << "utf-8"
                                             << videoUrl);

    connect(titleProcess, &QProcess::finished, this, [this, titleProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed while fetching video title!";
            titleProcess->deleteLater();
            return;
        }

        QString output = titleProcess->readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            qDebug() << "Video title:" << output;
            QFont font("Arial", 14, QFont::Bold);
            ui->l_vid_title->setFont(font);
            ui->l_vid_title->setText(output);
            QString f_videoTitle = output;
            videoTitle = f_videoTitle.replace(" ", "-");
        } else {
            qDebug() << "Failed to fetch title!";
        }

        titleProcess->deleteLater();
    });

    titleProcess->start();
}
QString MainWindow::getUrlType(const QString &videoUrl) {
    QUrl url(videoUrl);
    QUrlQuery query(url);
    QString path = url.path();
    if (query.hasQueryItem("list")) {
        return "playlist";
    } else if (path.contains("/channel/") || path.contains("/c/") || path.contains("/user/")) {
        return "channel";
    }
    return "video";
}

void MainWindow::loadImage(const QUrl &imageUrl) {
    if (imageUrl.isValid() && !imageUrl.isEmpty()) {
        QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
        QNetworkRequest request(imageUrl);
        QNetworkReply *reply = networkManager->get(request);

        disconnect(reply, &QNetworkReply::finished, this, nullptr);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QPixmap pixmap;
                if (pixmap.loadFromData(data)) {
                    int w = ui->imageLabel->width();
                    int h = ui->imageLabel->height();
                    ui->imageLabel->setPixmap(pixmap.scaled(w, h, Qt::KeepAspectRatio));
                    qDebug() << "Image uploaded successfully!";
                } else {
                    ui->imageLabel->setText("Error: Image cannot be loaded!");
                    qDebug() << "Error: Image cannot be loaded!";
                }
            } else {
                ui->imageLabel->setText("Error: " + reply->errorString());
                qDebug() << "Error:" << reply->errorString();
            }
            reply->deleteLater();
        });
    } else {
        ui->imageLabel->setText("Error: Wrong URL");
        qDebug() << "Error: Wrong URL";
    }
}

QString MainWindow::extractFormatId(const QString &selectedFormat) {
    return formatMap.value(selectedFormat, QString());
}
