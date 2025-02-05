#include "ProcessingDialog.h"
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QDebug>

ProcessingDialog::ProcessingDialog(QWidget* parent) 
    : QDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint) {
    setModal(true);

    mainLayout = new QVBoxLayout(this);
    
    // Main progress message
    messageLabel = new QLabel("Processing files...", this);
    messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(messageLabel);

    // Current file being processed
    currentFileLabel = new QLabel("Waiting to start...", this);
    currentFileLabel->setAlignment(Qt::AlignLeft);
    currentFileLabel->setWordWrap(true);
    mainLayout->addWidget(currentFileLabel);

    // Statistics label
    statisticsLabel = new QLabel("Files processed: 0\nTotal size: 0 bytes", this);
    statisticsLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(statisticsLabel);

    // Progress bar
    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    mainLayout->addWidget(progressBar);

    // Set a reasonable size for the dialog
    setFixedSize(500, 200);
    setWindowTitle("Processing");
}

void ProcessingDialog::setProgress(int current, int total) {
    if (total <= 0) return;
    
    int percentage = (current * 100) / total;
    progressBar->setValue(percentage);
    messageLabel->setText(QString("Processing files... (%1 of %2)").arg(current).arg(total));
    
    qDebug() << "Progress:" << current << "of" << total;
}

void ProcessingDialog::setCurrentFile(const QString& filePath) {
    QString displayPath = filePath;
    int lastSlash = displayPath.lastIndexOf('/');
    if (lastSlash == -1) lastSlash = displayPath.lastIndexOf('\\');
    
    if (lastSlash != -1) {
        QString fileName = displayPath.mid(lastSlash + 1);
        QString directory = displayPath.left(lastSlash);
        currentFileLabel->setText(QString("Current file: %1\nIn: %2").arg(fileName).arg(directory));
    } else {
        currentFileLabel->setText(QString("Current file: %1").arg(displayPath));
    }
    
    qDebug() << "Processing file:" << filePath;
}

QString ProcessingDialog::formatFileSize(qint64 size) const {
    const char* units[] = {"bytes", "KB", "MB", "GB"};
    int unitIndex = 0;
    double fileSize = size;

    while (fileSize >= 1024 && unitIndex < 3) {
        fileSize /= 1024;
        unitIndex++;
    }

    return QString("%1 %2")
        .arg(fileSize, 0, 'f', unitIndex > 0 ? 2 : 0)
        .arg(units[unitIndex]);
}

void ProcessingDialog::updateStatistics(int processedFiles, qint64 totalSize) {
    m_processedFiles = processedFiles;
    m_totalSize = totalSize;
    statisticsLabel->setText(QString("Files processed: %1\nTotal size: %2")
        .arg(processedFiles)
        .arg(formatFileSize(totalSize)));
    
    qDebug() << "Statistics update - Files:" << processedFiles << "Size:" << formatFileSize(totalSize);
}