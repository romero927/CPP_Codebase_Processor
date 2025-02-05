// MainWindow.h
// This header defines our main application window and declares all its functionality.
#pragma once

#include <QMainWindow>
#include <QString>
#include <set>

// Forward declarations to reduce header dependencies
class QTreeView;
class QPushButton;
class QVBoxLayout;
class QFileSystemWatcher;
class FileSystemModelWithGitIgnore;
class ProcessingDialog;
class QItemSelection;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectFolder();
    void saveToFile();
    void saveToClipboard();
    void onGitIgnoreChanged();
    void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void delayedInit();
    void setupUI();
    void expandDirectory(const QModelIndex& index, int depth);
    QString processFiles();
    void selectAllProcessableFiles(const QModelIndex& parentIndex);
    void expandEntireDirectoryTree(const QModelIndex& parentIndex, int depth = 0);
    void updateFileSelection(const QString& filePath, bool selected);

    // UI Elements
    QWidget *centralWidget{nullptr};
    QVBoxLayout *mainLayout{nullptr};
    QPushButton *selectFolderButton{nullptr};
    QTreeView *fileTreeView{nullptr};
    QPushButton *saveFileButton{nullptr};
    QPushButton *saveClipboardButton{nullptr};
    
    // Model and data handling
    FileSystemModelWithGitIgnore *fileModel{nullptr};
    QFileSystemWatcher *gitignoreWatcher{nullptr};
    QString currentPath;
    std::set<QString> selectedFiles;
    QThread* workerThread{nullptr};
    void startFileProcessing(bool toClipboard);
};
