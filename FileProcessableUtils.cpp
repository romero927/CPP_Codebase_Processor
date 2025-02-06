#include "FileProcessableUtils.h"
#include "FileExtensionConfig.h"
#include <QFileInfo>
#include <QDebug>

bool isFileProcessableImpl(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    
    // Get configuration references
    const auto& excludedDirs = FileExtensionConfig::getInstance().getExcludedDirectories();
    const auto& allowedExtensions = FileExtensionConfig::getInstance().getAllowedExtensions();
    qint64 maxSizeBytes = FileExtensionConfig::getInstance().getMaxFileSizeMB() * 1024 * 1024;

    // Explicitly reject certain directories
    QString dirName = fileInfo.fileName();
    if (excludedDirs.contains(dirName)) {
        return false;
    }
    
    // If it's a directory, return true
    if (fileInfo.isDir()) {
        return true;
    }
    
    // Check file size
    if (fileInfo.size() > maxSizeBytes) {
        qDebug() << "File exceeds max size:" << filePath 
                 << "Size:" << fileInfo.size() 
                 << "Max:" << maxSizeBytes;
        return false;
    }
    
    // Check file extension whitelist for text-based files
    QString ext = fileInfo.suffix().toLower();
    
    bool isProcessable = allowedExtensions.contains(ext);
    
    if (!isProcessable) {
        qDebug() << "File not processable:" << filePath 
                 << "Extension:" << ext;
    }
    
    return isProcessable;
}