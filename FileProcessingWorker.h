// FileProcessingWorker.h
#pragma once

#include <QObject>
#include <QString>
#include <set>

class FileSystemModelWithGitIgnore;

class FileProcessingWorker : public QObject {
    Q_OBJECT

public:
    explicit FileProcessingWorker(
        const QString& rootPath, 
        const std::set<QString>& selectedFiles,
        FileSystemModelWithGitIgnore* model,
        QObject* parent = nullptr
    );

public slots:
    void process();

signals:
    void processingProgress(int current, int total);
    void currentFile(const QString& filePath);
    void statistics(int processedFiles, qint64 totalSize);
    void finished(const QString& result);
    void error(const QString& message);

private:
    QString rootPath;
    std::set<QString> selectedFiles;
    FileSystemModelWithGitIgnore* fileModel;
    qint64 totalProcessedSize;
};