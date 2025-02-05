#include "FileSystemModelWithGitIgnore.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
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
    defaultIgnorePatterns = {
        ".vs/*",       // Visual Studio folder
        "build/*",     // Build output directory
        "out/*",       // Output directory
        "Debug/*",     // Debug build folder
        "Release/*",   // Release build folder
        "x64/*",       // Architecture-specific build folders
        "x86/*",
        "bin/*",       // Binary output directories
        "obj/*",       // Object file directories
        "dist/*",      // Distribution directory
        
        // Ignore compiled and binary files
        "*.exe",
        "*.dll",
        "*.obj",
        "*.pdb",
        "*.lib",
        "*.log",
        "*.cache",
        
        // IDE and system files
        "*.user",
        "*.suo",
        "*.sln",
        ".DS_Store",
        "Thumbs.db"
    };
    isInitialized = true;
}


Qt::ItemFlags FileSystemModelWithGitIgnore::flags(const QModelIndex& index) const {
    return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
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
    
    // Explicitly reject certain directories
    if (fileInfo.isDir()) {
        QString dirName = fileInfo.fileName();
        if (dirName == ".vs" || dirName == "build" || dirName == "out" || 
            dirName == "Debug" || dirName == "Release" || 
            dirName == "bin" || dirName == "obj") {
            return false;
        }
        return true;  // Allow other directories
    }
    
    // Check file extension whitelist for text-based files
    QString ext = fileInfo.suffix().toLower();
    QStringList allowedExtensions = {
        "cpp", "h", "hpp", "c", "txt", "md", "cmake", 
        "json", "yml", "yaml", "bat", "sh", "cs", 
        "py", "js", "ts", "html", "css", "xml", 
        "ini", "toml"
    };
    
    return !isPathIgnored(filePath) && allowedExtensions.contains(ext);
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

bool FileSystemModelWithGitIgnore::shouldIncludeFile(const QString& filePath) const {
    if (!isInitialized) {
        return true;
    }
    return isFileProcessable(filePath);
}