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
    // Если yt-dlp не в системном PATH, укажите полный путь.
    // downloadProcess->setProgram("C:/path/to/yt-dlp.exe");

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

        if (outputStr.contains("B/s")) {
            QStringList parts = outputStr.split(" ");
            for (const QString &part : parts) {
                if (part.endsWith("B/s")) {
                    qDebug() << "Download speed:" << part;
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

    if (urlType == "playlist") {
        qDebug() << "This is a playlist URL!";
    } else if (urlType == "channel") {
        qDebug() << "This is a channel URL!";
    } else if (urlType == "video") {
        qDebug() << "This is a single video URL!";
        fetchVideoDetails(videoUrl);
        fetchAvailableFormats(videoUrl);
    } else {
        qDebug() << "Unknown URL type!";
    }

    connect(ui->searchButton, &QPushButton::pressed, this, &MainWindow::on_searchButton_pressed);
}

void MainWindow::fetchAvailableFormats(const QString &videoUrl) {
    QProcess *formatProcess = new QProcess(this);
    formatProcess->setProgram("yt-dlp");
    formatProcess->setArguments(QStringList() << "-F"
                                              << videoUrl);

    disconnect(formatProcess, &QProcess::finished, this, nullptr);
    connect(formatProcess, &QProcess::finished, this, [this, formatProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed while fetching formats!";
            formatProcess->deleteLater();
            return;
        }

        QString output = formatProcess->readAllStandardOutput().trimmed();
        QStringList outputLines = output.split("\n");

        QStringList formats;
        for (const QString &line : outputLines) {

            if (line.contains("audio") || line.contains("video")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));

                if (parts.size() > 4) {
                    QString formatId = parts[0];
                    QString resolution = parts[1];
                    QString type = parts.last();

                    QString readableFormat = resolution + " " + parts[2] + " (" + type + ")";
                    formats.append(readableFormat);

                    formatMap[readableFormat] = formatId;
                }
            }
        }

        if (!formats.isEmpty()) {
            ui->comboBoxFormats->clear();
            ui->comboBoxFormats->addItems(formats);
        } else {
            qDebug() << "No formats found for this video!";
        }

        formatProcess->deleteLater();
    });

    formatProcess->start();
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
            videoTitle = output;
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

    if (query.hasQueryItem("list")) {
        return "playlist";
    }

    QString path = url.path();
    if (path.contains("/channel/") || path.contains("/c/") || path.contains("/user/")) {
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
