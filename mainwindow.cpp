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
    QString inputUrl = ui->e_inputURL->text();
    QString outputUrl = inputUrl;
    if (inputUrl.contains("www.youtube.com")){
        qDebug()<< "YouTube Detected";

        QRegularExpression regex("[?&]v=([^&][^?]+)");
        QRegularExpressionMatch match = regex.match(inputUrl);

        if(match.hasMatch()){
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
                qDebug() <<"Error: Image cannot be loaded!";
            }
            downloader->deleteLater();
        });
    } else {
        ui->Image->setText("Error: Wrong URL"); qDebug() << "Error: Wrong URL";
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



