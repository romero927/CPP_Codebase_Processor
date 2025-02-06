#include "FileProcessingWorker.h"
#include "FileSystemModelWithGitIgnore.h"
#include "FileProcessableUtils.h"
#include "FileExtensionConfig.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QDir>

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
    QDir baseDir(rootPath);
    
    // Filter out non-processable files first
    std::set<QString> processableFiles;
    for (const QString& filePath : selectedFiles) {
        if (isFileProcessableImpl(filePath)) {
            processableFiles.insert(filePath);
        }
    }
    
    // Use processable files for total count
    int totalFiles = processableFiles.size();
    emit processingProgress(0, totalFiles);

    // Detailed processing log
    qDebug() << "Starting to process" << totalFiles << "files";

    int processedFiles = 0;
    for (const QString& filePath : processableFiles) {
        // Emit current file being processed
        emit currentFile(filePath);
        
        QFile file(filePath);
        
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray content = file.readAll();
            
            // Get the relative path for output
            QString relativePath = baseDir.relativeFilePath(filePath);
            result += "=== " + relativePath + " ===\n";
            result += QString::fromUtf8(content);
            result += "\n\n";
            
            // Update total processed size
            totalProcessedSize += content.size();
            processedFiles++;
            
            // Update progress and statistics
            emit processingProgress(processedFiles, totalFiles);
            emit statistics(processedFiles, totalProcessedSize);
            
            // Give the UI a chance to update and prevent UI freezing
            QThread::msleep(1);
        } else {
            // Error handling for file open failures
            QString errorMessage = QString("Could not open file: %1 - %2")
                                    .arg(filePath, file.errorString());
            qWarning() << errorMessage;
            emit error(errorMessage);
            return;
        }
    }

    // Final checks and signaling
    if (result.isEmpty()) {
        qWarning() << "No files were processed.";
        emit error("No files were processed. Please check your selection.");
        return;
    }

    // Log successful processing
    qDebug() << "Successfully processed" << processedFiles << "files"
             << "Total size:" << totalProcessedSize << "bytes";

    // Signal successful completion
    emit finished(result);
}