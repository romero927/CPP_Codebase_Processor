#pragma once

#include <QDialog>
#include <QString>

class QProgressBar;
class QLabel;
class QVBoxLayout;

class ProcessingDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProcessingDialog(QWidget* parent = nullptr);
    int processedFiles() const { return m_processedFiles; }
    qint64 totalSize() const { return m_totalSize; }
    QString formatFileSize(qint64 size) const;

public slots:
    void setProgress(int current, int total);
    void setCurrentFile(const QString& filePath);
    void updateStatistics(int processedFiles, qint64 totalSize);

private:
    QVBoxLayout* mainLayout;
    QLabel* messageLabel;
    QLabel* currentFileLabel;
    QLabel* statisticsLabel;
    QProgressBar* progressBar;
    int m_processedFiles = 0;
    qint64 m_totalSize = 0;
};
