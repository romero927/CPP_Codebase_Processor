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
    // Clean up worker thread explicitly
    if (workerThread) {
        if (workerThread->isRunning()) {
            workerThread->quit();
            workerThread->wait(1000); // Wait up to 1 second for thread to terminate
        }
        
        // Additional safety checks
        if (workerThread->isRunning()) {
            qWarning() << "Worker thread did not terminate gracefully";
            workerThread->terminate(); // Last resort, but not recommended
        }
        
        delete workerThread;
        workerThread = nullptr;
    }

    // Cleanup UI components
    delete fileTreeView;
    delete selectFolderButton;
    delete saveFileButton;
    delete saveClipboardButton;
    delete mainLayout;
    delete centralWidget;

    // Cleanup models and watchers
    if (fileModel) {
        delete fileModel;
        fileModel = nullptr;
    }

    if (gitignoreWatcher) {
        delete gitignoreWatcher;
        gitignoreWatcher = nullptr;
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
            
            // Clear previous selection
            selectedFiles.clear();
            fileTreeView->selectionModel()->clearSelection();
            
            // Manually populate selection
            QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                if (fileModel->shouldIncludeFile(filePath)) {
                    selectedFiles.insert(filePath);
                    QModelIndex index = fileModel->index(filePath);
                    if (index.isValid()) {
                        fileTreeView->selectionModel()->select(
                            index,
                            QItemSelectionModel::Select | QItemSelectionModel::Rows
                        );
                    }
                    qDebug() << "Auto selecting:" << filePath;
                }
            }
            
            qDebug() << "Total auto-selected files:" << selectedFiles.size();

            // Expand the entire directory tree
            expandEntireDirectoryTree(rootIndex);

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
    if (depth > 10) return; // Prevent excessive recursion
    
    fileTreeView->expand(parentIndex);
    int rows = fileModel->rowCount(parentIndex);
    
    for (int i = 0; i < rows; ++i) {
        QModelIndex childIndex = fileModel->index(i, 0, parentIndex);
        if (fileModel->isDir(childIndex)) {
            expandEntireDirectoryTree(childIndex, depth + 1);
        }
        
        // Check if the file should be included and select it
        if (fileModel->shouldIncludeFile(fileModel->filePath(childIndex))) {
            fileTreeView->selectionModel()->select(childIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            selectedFiles.insert(fileModel->filePath(childIndex));
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
    // Validate that we have files to process
    if (selectedFiles.empty()) {
        QMessageBox::warning(this, "No Files Selected",
                             "Please select files to process first.");
        return;
    }

    // Ensure any previous worker thread is properly cleaned up
    if (workerThread) {
        if (workerThread->isRunning()) {
            workerThread->quit();
            if (!workerThread->wait(2000)) { // Wait up to 2 seconds
                qWarning() << "Worker thread did not terminate gracefully. Attempting to terminate.";
                workerThread->terminate(); // Last resort
            }
            delete workerThread;
            workerThread = nullptr;
        }
    }

    // Create a set of files that should actually be processed
    std::set<QString> filesToProcess;
    int processableFilesCount = 0;
    qint64 totalProcessableSize = 0;

    // Detailed file filtering with logging
    for (const QString& filePath : selectedFiles) {
        if (fileModel->shouldIncludeFile(filePath)) {
            QFileInfo fileInfo(filePath);
            
            if (fileInfo.isFile()) {
                try {
                    // Additional safety checks
                    if (!fileInfo.isReadable()) {
                        qWarning() << "File not readable:" << filePath;
                        continue;
                    }

                    filesToProcess.insert(filePath);
                    processableFilesCount++;
                    totalProcessableSize += fileInfo.size();
                } catch (const std::exception& e) {
                    qWarning() << "Error processing file:" << filePath 
                               << "Exception:" << e.what();
                }
            }
        }
    }

    // Validate processable files
    if (filesToProcess.empty()) {
        QMessageBox::warning(this, "No Valid Files",
                             "None of the selected items can be processed. "
                             "Please select valid files or check file extension configuration.");
        return;
    }

    // Log processing details
    qDebug() << "Processing " << processableFilesCount << " files"
             << "Total processable size:" << totalProcessableSize << "bytes"
             << "Destination:" << (toClipboard ? "Clipboard" : "File");

    // Optional: Confirm processing large files
    const qint64 LARGE_FILE_THRESHOLD_MB = 100; // 100 MB
    if (totalProcessableSize > (LARGE_FILE_THRESHOLD_MB * 1024 * 1024)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Large File Set", 
            QString("You are about to process %1 files totaling %2 MB. Continue?")
                .arg(processableFilesCount)
                .arg(totalProcessableSize / (1024 * 1024)),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }

    // Create processing dialog
    auto* dialog = new ProcessingDialog(this);
    dialog->setWindowTitle(toClipboard ? "Copying to Clipboard" : "Saving to File");
    dialog->setModal(true);
    dialog->show();
    qApp->processEvents();

    // Create worker thread with enhanced safety
    auto* worker = new FileProcessingWorker(currentPath, filesToProcess, fileModel);
    workerThread = new QThread(this);
    worker->moveToThread(workerThread);

    // Connect signals with error handling
    connect(worker, &FileProcessingWorker::processingProgress, 
            dialog, &ProcessingDialog::setProgress);
    connect(worker, &FileProcessingWorker::currentFile,
            dialog, &ProcessingDialog::setCurrentFile);
    connect(worker, &FileProcessingWorker::statistics,
            dialog, &ProcessingDialog::updateStatistics);

    // Handle successful completion
    connect(worker, &FileProcessingWorker::finished, this, 
        [this, dialog, worker, toClipboard, processableFilesCount](const QString& result) {
            // Ensure UI updates happen on main thread
            QMetaObject::invokeMethod(this, [this, dialog, worker, toClipboard, result, processableFilesCount]() {
                // Clean up dialog first
                dialog->hide();
                dialog->deleteLater();
    
                // Clean up worker and thread with proper order
                worker->deleteLater();
                if (workerThread) {
                    workerThread->quit();
                    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
                    connect(workerThread, &QThread::finished, [this]() {
                        workerThread = nullptr;
                    });
                }

                // Get the final statistics
                int actualProcessedFiles = dialog->processedFiles();
                QString totalSize = dialog->formatFileSize(dialog->totalSize());

                if (result.isEmpty()) {
                    QMessageBox::warning(this, "Processing Result", 
                                       "No content was processed. Please check your file selection.");
                    return;
                }

                if (toClipboard) {
                    QClipboard* clipboard = QApplication::clipboard();
                    
                    // Set text in the regular clipboard
                    clipboard->setText(result, QClipboard::Clipboard);
                    
                    // Also set it in the X11 primary selection for Linux
                    if (clipboard->supportsSelection()) {
                        clipboard->setText(result, QClipboard::Selection);
                    }
                    
                    // Force event processing to ensure clipboard content is properly set
                    QApplication::processEvents();
                    
                    QMessageBox::information(this, "Success",
                        QString("Content copied to clipboard successfully!\n\n"
                                "Files processed: %1\nTotal size: %2")
                        .arg(actualProcessedFiles).arg(totalSize));
                } else {
                    // Create a default filename
                    QString defaultFileName = QFileInfo(currentPath).fileName() + "_processed.txt";
                    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
                    QString defaultFilePath = QDir(defaultPath).filePath(defaultFileName);

                    QString savePath = QFileDialog::getSaveFileName(
                        this,
                        "Save Processed Code",
                        defaultFilePath,
                        "Text Files (*.txt);;Markdown Files (*.md);;All Files (*.*)"
                    );

                    if (!savePath.isEmpty()) {
                        QFile file(savePath);
                        
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                            // Set file permissions after opening
                            file.setPermissions(QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ReadUser | QFile::WriteUser |
                                              QFile::ReadGroup | QFile::ReadOther);
                            // Write UTF-8 BOM
                            file.write("\xEF\xBB\xBF");
                            // Write content as UTF-8
                            file.write(result.toUtf8());
                            file.flush();
                            file.close();
                            QMessageBox::information(this, "Success",
                                QString("Files successfully processed and saved!\n\n"
                                        "Files processed: %1\nTotal size: %2")
                                .arg(actualProcessedFiles).arg(totalSize));
                        } else {
                            QMessageBox::critical(this, "Error",
                                "Could not save the file: " + file.errorString());
                        }
                    }
                }

                dialog->deleteLater();
            });
        }
    );

    // Handle processing errors
    connect(worker, &FileProcessingWorker::error, this, 
        [this, dialog, worker](const QString& message) {
            // Ensure UI updates happen on main thread
            QMetaObject::invokeMethod(this, [this, dialog, worker, message]() {
                // Clean up dialog first
                dialog->hide();
                dialog->deleteLater();

                // Clean up worker and thread with proper order
                worker->deleteLater();
                if (workerThread) {
                    workerThread->quit();
                    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
                    connect(workerThread, &QThread::finished, [this]() {
                        workerThread = nullptr;
                    });
                }
                
                QMessageBox::critical(this, "Processing Error", message);
            });
        }
    );

    // Ensure proper cleanup if thread fails to start
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Start processing
    connect(workerThread, &QThread::started, worker, &FileProcessingWorker::process);
    
    // Start the thread
    try {
        workerThread->start();
        qDebug() << "Processing thread started successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to start processing thread:" << e.what();
        QMessageBox::critical(this, "Thread Error", 
            "Could not start processing thread. Please try again.");
        
        // Cleanup
        dialog->hide();
        dialog->deleteLater();
        
        if (workerThread) {
            delete workerThread;
            workerThread = nullptr;
        }
        
        worker->deleteLater();
    }
}

void MainWindow::saveToClipboard()
{
    startFileProcessing(true);
}

void MainWindow::saveToFile()
{
    startFileProcessing(false);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Check if there's an active worker thread
    if (workerThread && workerThread->isRunning()) {
        // Prompt user about ongoing processing
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Ongoing Processing", 
            "A file processing task is currently running. Do you want to stop it and close the application?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            // Cancel the close event
            event->ignore();
            return;
        }
        
        // Attempt to stop the thread
        workerThread->quit();
        
        // Wait for the thread to terminate
        if (!workerThread->wait(2000)) { // Wait up to 2 seconds
            qWarning() << "Worker thread did not terminate gracefully. Attempting to terminate.";
            workerThread->terminate(); // Last resort
        }
    }

    // Cleanup any remaining resources
    if (fileModel) {
        delete fileModel;
        fileModel = nullptr;
    }

    if (gitignoreWatcher) {
        delete gitignoreWatcher;
        gitignoreWatcher = nullptr;
    }

    // Clear selected files
    selectedFiles.clear();

    // Accept the close event
    event->accept();

    // Call base class implementation
    QMainWindow::closeEvent(event);
}