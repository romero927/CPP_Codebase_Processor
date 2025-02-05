#include "MainWindow.h"
#include "FileSystemModelWithGitIgnore.h"
#include "ProcessingDialog.h"
#include "FileProcessingWorker.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>
#include <QClipboard>
#include <QApplication>
#include <QFileSystemWatcher>
#include <QThread>
#include <QTimer>
#include <QStandardPaths>
#include <QItemSelectionModel>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent)
    , workerThread(nullptr)
{
    // Only do minimal setup initially
    setWindowTitle("Codebase Processor");
    resize(800, 600);
    
    // Use QTimer to defer the rest of the initialization
    QTimer::singleShot(0, this, &MainWindow::delayedInit);
}

void MainWindow::delayedInit()
{
    setupUI();
    
    // Additional setup for file model
    fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
}

MainWindow::~MainWindow()
{
    // If there's an active worker thread, clean it up
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);

    // Create folder selection button
    selectFolderButton = new QPushButton("Select Codebase Folder", this);
    connect(selectFolderButton, &QPushButton::clicked, this, &MainWindow::selectFolder);
    mainLayout->addWidget(selectFolderButton);

    // Create tree view but defer model setup
    fileTreeView = new QTreeView(this);
    fileTreeView->setUniformRowHeights(true);
    fileTreeView->setEnabled(false);
    fileTreeView->setSortingEnabled(false); // Enable only after folder selection
    // Make sure multi-selection is enabled so multiple files can be selected
    fileTreeView->setSelectionMode(QAbstractItemView::MultiSelection);
    fileTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(fileTreeView);

    // Create action buttons
    saveFileButton = new QPushButton("Save to File", this);
    saveClipboardButton = new QPushButton("Copy to Clipboard", this);
    
    connect(saveFileButton, &QPushButton::clicked, this, &MainWindow::saveToFile);
    connect(saveClipboardButton, &QPushButton::clicked, this, &MainWindow::saveToClipboard);

    mainLayout->addWidget(saveFileButton);
    mainLayout->addWidget(saveClipboardButton);

    // Disable buttons initially
    saveFileButton->setEnabled(false);
    saveClipboardButton->setEnabled(false);

    // Initialize file system model
    fileModel = new FileSystemModelWithGitIgnore(this);
    fileModel->setReadOnly(true);
    fileTreeView->setModel(fileModel);

    // Set up selection handling after model is set
    connect(fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::handleSelectionChanged);

    // Set up gitignore watcher
    gitignoreWatcher = new QFileSystemWatcher(this);
    connect(gitignoreWatcher, &QFileSystemWatcher::fileChanged,
            this, &MainWindow::onGitIgnoreChanged);
}

void MainWindow::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Codebase Directory",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        
        // Use a timer to allow the UI to update
        QTimer::singleShot(0, this, [this, dir]() {
            currentPath = dir;
            
            // Set up the model with the new path
            QModelIndex rootIndex = fileModel->setRootPath(dir);
            fileTreeView->setRootIndex(rootIndex);
            
            // Update .gitignore patterns if present
            QString gitignorePath = dir + "/.gitignore";
            if (QFile::exists(gitignorePath)) {
                gitignoreWatcher->addPath(gitignorePath);
                fileModel->updateGitIgnorePatterns(dir);
            }
            
            // Expand the entire directory tree
            expandEntireDirectoryTree(rootIndex);

            // Manually populate selection
            QDir directory(dir);
            QStringList files = directory.entryList(
                QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, 
                QDir::Name
            );
            
            selectedFiles.clear();
            for (const QString& fileName : files) {
                QString fullPath = dir + QDir::separator() + fileName;
                if (fileModel->shouldIncludeFile(fullPath)) {
                    selectedFiles.insert(fullPath);
                }
            }
            
            // Programmatically select these files in the tree
            fileTreeView->selectionModel()->clearSelection();
            for (const QString &filePath : selectedFiles) {
                QModelIndex index = fileModel->index(filePath);
                if (index.isValid()) {
                    fileTreeView->selectionModel()->select(
                        index,
                        QItemSelectionModel::Select | QItemSelectionModel::Rows
                    );
                }
            }
            
            // Enable UI elements
            fileTreeView->setEnabled(true);
            saveFileButton->setEnabled(true);
            saveClipboardButton->setEnabled(true);
            fileTreeView->setSortingEnabled(true);
            
            QApplication::restoreOverrideCursor();
        });
    }
}

void MainWindow::expandEntireDirectoryTree(const QModelIndex& parentIndex, int depth)
{
    // Prevent excessive recursion
    if (depth > 10) return;
    
    // Expand the current index
    fileTreeView->expand(parentIndex);
    
    // Get the number of rows (child items) for this parent
    int rows = fileModel->rowCount(parentIndex);
    
    // Iterate through all child items
    for (int i = 0; i < rows; ++i) {
        QModelIndex childIndex = fileModel->index(i, 0, parentIndex);
        
        // If this is a directory, recursively expand it
        if (fileModel->isDir(childIndex)) {
            expandEntireDirectoryTree(childIndex, depth + 1);
        }
    }
}

void MainWindow::handleSelectionChanged(const QItemSelection& selected, 
                                        const QItemSelection& deselected)
{
    // Process newly selected items
    for (const QModelIndex& index : selected.indexes()) {
        if (index.column() == 0) {  // Only process the first column
            QString filePath = fileModel->filePath(index);
            // Check if it's a file
            QFileInfo fileInfo(filePath);
            if (fileInfo.isFile()) {
                selectedFiles.insert(filePath);
                qDebug() << "Added to selection:" << filePath;
            }
        }
    }
    
    // Process deselected items
    for (const QModelIndex& index : deselected.indexes()) {
        if (index.column() == 0) {  // Only process the first column
            QString filePath = fileModel->filePath(index);
            selectedFiles.erase(filePath);
            qDebug() << "Removed from selection:" << filePath;
        }
    }
    
    qDebug() << "Total files selected:" << selectedFiles.size();
}

void MainWindow::expandDirectory(const QModelIndex& index, int depth)
{
    if (depth <= 0) return;
    
    fileTreeView->expand(index);
    int rows = fileModel->rowCount(index);
    
    for (int i = 0; i < rows; ++i) {
        QModelIndex childIndex = fileModel->index(i, 0, index);
        if (fileModel->isDir(childIndex)) {
            expandDirectory(childIndex, depth - 1);
        }
    }
}

void MainWindow::onGitIgnoreChanged()
{
    // When .gitignore changes, update the patterns and refresh the view
    fileModel->updateGitIgnorePatterns(currentPath);
}

void MainWindow::startFileProcessing(bool toClipboard)
{
    // Debug output to help diagnose selection issues
    qDebug() << "Starting processing with" << selectedFiles.size() << "files selected";
    for (const QString& file : selectedFiles) {
        qDebug() << "Selected file:" << file;
    }

    // Validate that we have files to process
    if (selectedFiles.empty()) {
        QMessageBox::warning(this, "No Files Selected",
                           "Please select files to process first.");
        return;
    }

    // Create a set of files that should actually be processed
    std::set<QString> filesToProcess;
    for (const QString& filePath : selectedFiles) {
        if (fileModel->shouldIncludeFile(filePath)) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.isFile()) {
                filesToProcess.insert(filePath);
                qDebug() << "Will process:" << filePath;
            }
        }
    }

    if (filesToProcess.empty()) {
        QMessageBox::warning(this, "No Valid Files",
                           "None of the selected items can be processed. Please select valid files.");
        return;
    }

    // Create processing dialog
    auto* dialog = new ProcessingDialog(this);
    dialog->setWindowTitle(toClipboard ? "Copying to Clipboard" : "Saving to File");
    dialog->show();
    qApp->processEvents();

    // Create worker thread
    auto* worker = new FileProcessingWorker(currentPath, filesToProcess, fileModel);
    workerThread = new QThread(this);
    worker->moveToThread(workerThread);

    // Connect all signals
    connect(worker, &FileProcessingWorker::processingProgress, 
            dialog, &ProcessingDialog::setProgress);
    connect(worker, &FileProcessingWorker::currentFile,
            dialog, &ProcessingDialog::setCurrentFile);
    connect(worker, &FileProcessingWorker::statistics,
            dialog, &ProcessingDialog::updateStatistics);

    // Handle completion
    connect(worker, &FileProcessingWorker::finished, this, 
            [this, dialog, worker, toClipboard](const QString& result) {
        // Clean up worker thread
        workerThread->quit();
        worker->deleteLater();
        dialog->hide();

        // Get the final statistics
        int processedFiles = dialog->processedFiles() ;
        QString totalSize = dialog->formatFileSize(dialog->totalSize());

        if (result.isEmpty()) {
            QMessageBox::warning(this, "Processing Result", 
                               "No content was processed. Please check your file selection.");
        } else if (toClipboard) {
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(result);
            QMessageBox::information(this, "Success",
                QString("Content copied to clipboard successfully!\n\n"
                        "Files processed: %1\nTotal size: %2")
                .arg(processedFiles).arg(totalSize));
        } else {
            // Create a default filename
            QString defaultFileName = QFileInfo(currentPath).fileName() + "_processed.txt";
            QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            QString defaultFilePath = QDir(defaultPath).filePath(defaultFileName);

            QString savePath = QFileDialog::getSaveFileName(
                this,
                "Save Processed Code",
                defaultFilePath,
                "Text Files (*.txt);;Markdown Files (*.md);;All Files (*.*)",
                nullptr,
                QFileDialog::DontUseNativeDialog
            );

            if (!savePath.isEmpty()) {
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << result;
                    file.close();
                    QMessageBox::information(this, "Success",
                        QString("Files successfully processed and saved!\n\n"
                                "Files processed: %1\nTotal size: %2")
                        .arg(processedFiles).arg(totalSize));
                } else {
                    QMessageBox::critical(this, "Error",
                        "Could not save the file: " + file.errorString());
                }
            }
        }

        dialog->deleteLater();
    });

    // Handle errors
    connect(worker, &FileProcessingWorker::error, this, 
            [this, dialog](const QString& message) {
        dialog->hide();
        dialog->deleteLater();
        QMessageBox::critical(this, "Error", message);
    });

    // Ensure proper cleanup
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Start processing
    connect(workerThread, &QThread::started, worker, &FileProcessingWorker::process);
    workerThread->start();

    qDebug() << "Processing thread started";
}

void MainWindow::saveToClipboard()
{
    startFileProcessing(true);
}

void MainWindow::saveToFile()
{
    startFileProcessing(false);
}