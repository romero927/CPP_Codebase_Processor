#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStringList>


class FileExtensionConfig {
public:
    static FileExtensionConfig& getInstance() {
        static FileExtensionConfig instance;
        return instance;
    }

    QStringList getAllowedExtensions() const { return m_textExtensions; }
    QStringList getExcludedDirectories() const { return m_excludedDirectories; }
    qint64 getMaxFileSizeMB() const { return m_maxFileSizeMB; }

private:
    FileExtensionConfig() {
        loadConfig();
    }

    void loadConfig() {
        QFile configFile(":/config/file_extensions.json");
        if (!configFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file extensions config file";
            return;
        }

        QJsonDocument jsonDoc = QJsonDocument::fromJson(configFile.readAll());
        if (!jsonDoc.isObject()) {
            qWarning() << "Invalid JSON configuration";
            return;
        }

        QJsonObject configObj = jsonDoc.object();

        // Parse text extensions
        QJsonArray extensionsArray = configObj["text_extensions"].toArray();
        for (const QJsonValue& ext : extensionsArray) {
            m_textExtensions.append(ext.toString());
        }

        // Parse excluded directories
        QJsonArray dirArray = configObj["excluded_directories"].toArray();
        for (const QJsonValue& dir : dirArray) {
            m_excludedDirectories.append(dir.toString());
        }

        // Parse max file size
        m_maxFileSizeMB = configObj["max_file_size_mb"].toInt(10);
    }

    QStringList m_textExtensions;
    QStringList m_excludedDirectories;
    qint64 m_maxFileSizeMB;
};