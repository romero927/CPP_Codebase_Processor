#include "FileProcessingWorker.h"
#include "FileSystemModelWithGitIgnore.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>
#include <QThread>  // Added for QThread::msleep

FileProcessingWorker::FileProcessingWorker(
    const QString& path, 
    const std::set<QString>& files,
    FileSystemModelWithGitIgnore* model,
    QObject* parent
) : QObject(parent)
  , rootPath(path)
  , selectedFiles(files)
  , fileModel(model)
  , totalProcessedSize(0) {
}

void FileProcessingWorker::process() {
    QString result;
    
    // Filter out non-processable files first
    std::set<QString> processableFiles;
    for (const QString& filePath : selectedFiles) {
        if (fileModel->shouldIncludeFile(filePath)) {
            processableFiles.insert(filePath);
        }
    }
    
    // Use processable files for total count
    int totalFiles = processableFiles.size();
    emit processingProgress(0, totalFiles);

    int processedFiles = 0;
    for (const QString& filePath : processableFiles) {
        emit currentFile(filePath);
        QFile file(filePath);
        
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray content = file.readAll();
            result += "=== " + filePath + " ===\n";
            result += QString::fromUtf8(content);
            result += "\n\n";
            
            totalProcessedSize += content.size();
            processedFiles++;
            
            // Update progress and statistics
            emit processingProgress(processedFiles, totalFiles);
            emit statistics(processedFiles, totalProcessedSize);
            
            // Give the UI a chance to update
            QThread::msleep(1);
        } else {
            emit error(QString("Could not open file: %1 - %2")
                      .arg(filePath)
                      .arg(file.errorString()));
            return;
        }
    }

    if (result.isEmpty()) {
        emit error("No files were processed. Please check your selection.");
        return;
    }

    emit finished(result);
}