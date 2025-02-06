// FileSystemModelWithGitIgnore.h
#pragma once

#include <QFileSystemModel>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

class FileSystemModelWithGitIgnore : public QFileSystemModel {
    Q_OBJECT

public:
    explicit FileSystemModelWithGitIgnore(QObject* parent = nullptr);
    ~FileSystemModelWithGitIgnore();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void updateGitIgnorePatterns(const QString& rootPath);
    bool shouldIncludeFile(const QString& filePath) const;

private:
    QStringList gitIgnorePatterns;
    QStringList defaultIgnorePatterns;
    QString projectRootPath;
    bool isInitialized;

    void initializeDefaultPatterns();
    bool isPathIgnored(const QString& path) const;
    QString getRelativePath(const QString& path) const;
    
    // Declare the method in the header
    bool isFileProcessable(const QString& filePath) const;
};