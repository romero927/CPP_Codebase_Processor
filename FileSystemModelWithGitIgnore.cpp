#include "FileSystemModelWithGitIgnore.h"
#include "FileExtensionConfig.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

FileSystemModelWithGitIgnore::FileSystemModelWithGitIgnore(QObject* parent)
    : QFileSystemModel(parent)
    , isInitialized(false) {
    // Defer initialization of patterns until they're actually needed
    setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
}

FileSystemModelWithGitIgnore::~FileSystemModelWithGitIgnore() {
}

void FileSystemModelWithGitIgnore::initializeDefaultPatterns() {
    // Get excluded directories from configuration
    defaultIgnorePatterns = FileExtensionConfig::getInstance().getExcludedDirectories().toVector();
    isInitialized = true;
}

void FileSystemModelWithGitIgnore::updateGitIgnorePatterns(const QString& rootPath) {
    if (!isInitialized) {
        initializeDefaultPatterns();
    }

    projectRootPath = rootPath;
    gitIgnorePatterns.clear();
    
    QFile gitignore(rootPath + "/.gitignore");
    if (gitignore.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&gitignore);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#')) {
                if (line.endsWith("/")) {
                    line += "*";
                }
                gitIgnorePatterns.append(line);
            }
        }
        gitignore.close();
    }
}

QString FileSystemModelWithGitIgnore::getRelativePath(const QString& path) const {
    return QDir(projectRootPath).relativeFilePath(path);
}

bool FileSystemModelWithGitIgnore::isPathIgnored(const QString& path) const {
    QString relativePath = getRelativePath(path);
    
    // Check default patterns first
    for (const QString& pattern : defaultIgnorePatterns) {
        QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(pattern),
                            QRegularExpression::CaseInsensitiveOption);
        if (rx.match(relativePath).hasMatch()) {
            return true;
        }
    }

    // Then check gitignore patterns
    for (const QString& pattern : gitIgnorePatterns) {
        QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(pattern),
                            QRegularExpression::CaseInsensitiveOption);
        if (rx.match(relativePath).hasMatch()) {
            return true;
        }
    }

    return false;
}

bool FileSystemModelWithGitIgnore::isFileProcessable(const QString& filePath) const {
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
        return false;
    }
    
    // Check file extension whitelist for text-based files 
    QString ext = fileInfo.suffix().toLower();
    return !isPathIgnored(filePath) && allowedExtensions.contains(ext);
}

bool FileSystemModelWithGitIgnore::shouldIncludeFile(const QString& filePath) const
{
    if (!isInitialized) {
        return true;
    }
    
    QFileInfo fileInfo(filePath);
    
    // Get configuration references
    const auto& excludedDirs = FileExtensionConfig::getInstance().getExcludedDirectories();
    const auto& allowedExtensions = FileExtensionConfig::getInstance().getAllowedExtensions();
    qint64 maxSizeBytes = FileExtensionConfig::getInstance().getMaxFileSizeMB() * 1024 * 1024;

    // Check if the file is in any excluded directory
    QString relativePath = QDir(projectRootPath).relativeFilePath(filePath);
    QStringList pathParts = relativePath.split('/');
    for (const QString& part : pathParts) {
        if (excludedDirs.contains(part)) {
            qDebug() << "Excluded directory:" << filePath;
            return false;
        }
    }
    
    // If it's a directory, return true to allow navigation
    if (fileInfo.isDir()) {
        return true;
    }
    
    // Check if the file matches any ignore patterns
    if (isPathIgnored(filePath)) {
        qDebug() << "Ignored by patterns:" << filePath;
        return false;
    }
    
    // Check file size
    if (fileInfo.size() > maxSizeBytes) {
        qDebug() << "File too large:" << filePath;
        return false;
    }
    
    // Check file extension whitelist for text-based files
    QString ext = fileInfo.suffix().toLower();
    
    bool isIncluded = allowedExtensions.contains(ext);
    if (isIncluded) {
        qDebug() << "Including file:" << filePath;
    } else {
        qDebug() << "Excluding file (extension not allowed):" << filePath;
    }
    
    return isIncluded;
}


Qt::ItemFlags FileSystemModelWithGitIgnore::flags(const QModelIndex& index) const {
    return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
}

QVariant FileSystemModelWithGitIgnore::data(const QModelIndex& index, int role) const {
    if (!isInitialized || !index.isValid()) {
        return QFileSystemModel::data(index, role);
    }

    if (role == Qt::CheckStateRole && index.column() == 0) {
        QString path = filePath(index);
        QFileInfo fileInfo(path);
        
        // Don't show checkboxes for directories
        if (fileInfo.isDir()) {
            return QVariant();
        }
        
        // Allow manual selection of all files, regardless of .gitignore rules
        return QFileSystemModel::data(index, role);
    }
    return QFileSystemModel::data(index, role);
}