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

    ui->downloadButton->hide();
    ui->comboBoxFormats->hide();

    QUrl imageUrl("http://qt.digia.com/Documents/1/QtLogo.png");
    connect(ui->downloadButton, &QPushButton::pressed, this, &MainWindow::on_downloadButton_pressed);
    connect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_downloadButton_pressed() {
    disconnect(ui->downloadButton, &QPushButton::pressed, this, &MainWindow::on_downloadButton_pressed);
    qDebug() << "Download button pressed!";

    QProcess *downloadProcess = new QProcess(this);
    downloadProcess->setProgram("yt-dlp");

    QString videoUrl = ui->e_inputUrl->text();
    QString savePath = QFileDialog::getSaveFileName(this, "Save Video", videoTitle, "All Files (*)");

    if (savePath.isEmpty()) {
        qDebug() << "Download canceled, no file selected!";
        connect(ui->downloadButton, &QPushButton::pressed, this, &MainWindow::on_downloadButton_pressed);
        return;
    }

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
    arguments << "-f" << formatId
              << "--output" << savePath
              << videoUrl
              << "--progress"
              << "--quiet";
    downloadProcess->setArguments(arguments);

    connect(downloadProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleProgress);
    connect(downloadProcess, &QProcess::finished, this, &MainWindow::downloadFinished);

    downloadProcess->start();
}

void MainWindow::handleProgress() {
    double lastProgress = 0;
    QProcess *downloadProcess = qobject_cast<QProcess *>(sender());
    if (!downloadProcess) return;

    QByteArray output = downloadProcess->readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);

    QRegularExpression progressRegex(R"(\s*(\d+(\.\d+)?)%)");
    QRegularExpressionMatch match = progressRegex.match(outputStr);

    if (match.hasMatch()) {
        double progress = match.captured(1).toDouble();

        if (qAbs(progress - lastProgress) >= 1.0) {
            lastProgress = progress;

            int step = (progress >= 100.0) ? 2 : 1;
            QString progressMessage = QString("(%1/2) Downloading: %2%").arg(step).arg(static_cast<int>(progress));
            statusBar()->showMessage(progressMessage);
        }
    }
}

void MainWindow::downloadFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess *downloadProcess = qobject_cast<QProcess *>(sender());
    if (!downloadProcess) return;

    if (exitStatus == QProcess::CrashExit) {
        qDebug() << "yt-dlp crashed!";
        statusBar()->showMessage("Download failed: yt-dlp crashed!");
    } else {
        qDebug() << "Download finished with exit code:" << exitCode;
        statusBar()->showMessage("Download complete!");
    }

    QByteArray errorOutput = downloadProcess->readAllStandardError();
    if (!errorOutput.isEmpty()) {
        qDebug() << "Error output:" << errorOutput;
    }

    downloadProcess->deleteLater();
    Sleep(1000);
    statusBar()->clearMessage();
    connect(ui->downloadButton, &QPushButton::pressed, this, &MainWindow::on_downloadButton_pressed);
}

void MainWindow::on_searchButton_pressed() {
    disconnect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);
    QString videoUrl = ui->e_inputUrl->text();
    QString urlType = getUrlType(videoUrl);

    statusBar()->showMessage("Searching...");

    qDebug() << "URL type = " << urlType;

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

        statusBar()->clearMessage();

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
        auto resolutionComparator = [](const QString &a, const QString &b) {
            QStringList resA = a.split("x");
            QStringList resB = b.split("x");

            if (resA.size() < 2 || resB.size() < 2)
                return false;

            int widthA = resA[0].toInt();
            int heightA = resA[1].toInt();
            int widthB = resB[0].toInt();
            int heightB = resB[1].toInt();

            if (widthA != widthB)
                return widthA > widthB;
            return heightA > heightB;
        };

        QStringList videoKeys = videoFormats.keys();
        QStringList audioKeys = audioFormats.keys();

        std::sort(videoKeys.begin(), videoKeys.end(), resolutionComparator);
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
            ui->comboBoxFormats->addItems(formats);
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

            ui->l_vid_title->setText(output);
            QString f_videoTitle = output;

            QFont font = ui->l_vid_title->font();
            font.setPointSize(16);
            ui->l_vid_title->setFont(font);

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
    ui->downloadButton->show();
    ui->comboBoxFormats->show();
}

QString MainWindow::extractFormatId(const QString &selectedFormat) {
    return formatMap.value(selectedFormat, QString());
}
