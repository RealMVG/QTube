#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filedownloader.h"


#ifdef _WIN64
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
#ifdef _WIN64
    qDebug() << "Windows!";
#else
    qDebug() << "Unix!";
#endif

    QUrl imageUrl("http://qt.digia.com/Documents/1/QtLogo.png");
    FileDownloader* downloader  = new FileDownloader(imageUrl, this);

    connect(ui->b_search, &QPushButton::pressed, this, &MainWindow::on_b_search_pressed);
    disconnect(ui->b_search, &QPushButton::pressed, this, &MainWindow::on_b_search_pressed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_b_search_pressed() {
    QString videoUrl = ui->e_inputURL->text();
    QString urlType = getUrlType(videoUrl);

    if (urlType == "playlist") {
        qDebug() << "This is a playlist URL!";
    } else if (urlType == "channel") {
        qDebug() << "This is a channel URL!";
    } else if (urlType == "video") {
        qDebug() << "This is a single video URL!";
        fetchVideoTitle(videoUrl);
    } else {
        qDebug() << "Unknown URL type!";
    }
}

void MainWindow::fetchVideoTitle(const QString &videoUrl) {

    QProcess *process = new QProcess(this);
    process->setProgram("yt-dlp");
    process->setArguments(QStringList() << "--quiet" << "--get-title" << "--encoding" << "utf-8" << videoUrl);

    connect(process, &QProcess::finished, this, [this, process, videoUrl](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::CrashExit) {
            qDebug() << "yt-dlp crashed!";
            process->deleteLater();
            return;
        }

        QString videoTitle = process->readAllStandardOutput().trimmed();
        if (videoTitle.isEmpty()) {
            qDebug() << "No title found for the video!";
        } else {
            qDebug() << "Video Title:" << videoTitle;
            ui->l_vid_name->setText(videoTitle);
        }

        process->deleteLater();

        handleImageUrl(videoUrl);
    });

    process->start();
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


void MainWindow::handleImageUrl(const QString &videoUrl) {
    QString outputUrl = videoUrl;
    if (videoUrl.contains("www.youtube.com")) {
        qDebug() << "YouTube Detected";

        QRegularExpression regex("[?&]v=([^&][^?]+)");
        QRegularExpressionMatch match = regex.match(videoUrl);

        if (match.hasMatch()) {
            QString videoId = match.captured(1);
            qDebug() << "Video ID:" << videoId;
            outputUrl = "https://img.youtube.com/vi/" + videoId + "/maxresdefault.jpg";
            qDebug() << outputUrl;
        } else {
            qDebug() << "Video ID not found.";
        }
    }

    QUrl imageUrl(outputUrl);

    if (imageUrl.isValid() && !imageUrl.isEmpty()) {
        FileDownloader* downloader = new FileDownloader(imageUrl, this);
        connect(downloader, &FileDownloader::downloaded, this, [this, downloader]() {
            QPixmap pixmap;
            if (pixmap.loadFromData(downloader->downloadedData())) {
                int w = ui->Image->width();
                int h = ui->Image->height();
                ui->Image->setPixmap(pixmap.scaled(w, h, Qt::KeepAspectRatio));
                qDebug() << "Image uploaded successfully!";
            } else {
                ui->Image->setText("Error: Image cannot be loaded!");
                qDebug() << "Error: Image cannot be loaded!";
            }
            downloader->deleteLater();
        });
    } else {
        ui->Image->setText("Error: Wrong URL");
        qDebug() << "Error: Wrong URL";
    }
}

FileDownloader::FileDownloader(QUrl imageUrl, QObject *parent) :
    QObject(parent)
{
    connect(
        &m_WebCtrl, SIGNAL (finished(QNetworkReply*)),
        this, SLOT (fileDownloaded(QNetworkReply*))
        );

    QNetworkRequest request(imageUrl);
    m_WebCtrl.get(request);
}

FileDownloader::~FileDownloader() { }

void FileDownloader::fileDownloaded(QNetworkReply* pReply) {
    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();
    emit downloaded();
}

QByteArray FileDownloader::downloadedData() const {
    return m_DownloadedData;
}
