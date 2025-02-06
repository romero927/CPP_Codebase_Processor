// main.cpp
// Application entry point with enhanced initialization and error handling

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QLoggingCategory>
#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

#include "MainWindow.h"

// Custom message handler for logging
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Determine log file path
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logPath); // Ensure log directory exists
    QString logFilePath = QDir(logPath).filePath("codebase_processor.log");

    // Open log file in append mode
    QFile logFile(logFilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // Fallback if can't open log file
        return;
    }

    QTextStream out(&logFile);
    
    // Format log message
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logLevel;

    // Determine log level
    switch (type) {
        case QtDebugMsg:
            logLevel = "DEBUG";
            break;
        case QtInfoMsg:
            logLevel = "INFO";
            break;
        case QtWarningMsg:
            logLevel = "WARNING";
            break;
        case QtCriticalMsg:
            logLevel = "CRITICAL";
            break;
        case QtFatalMsg:
            logLevel = "FATAL";
            break;
    }

    // Format log entry
    QString logEntry = QString("%1 [%2] %3 (File: %4, Line: %5, Function: %6)")
        .arg(timestamp, logLevel, msg, 
             context.file ? context.file : "Unknown", 
             QString::number(context.line),
             context.function ? context.function : "Unknown");

    // Write to log file
    out << logEntry << "\n";
    out.flush();

    // If it's a critical or fatal message, also show a message box
    if (type >= QtCriticalMsg) {
        QMessageBox::critical(nullptr, "Application Error", msg);
    }

    // Default handler for console output (optional)
    fprintf(stderr, "%s\n", logEntry.toUtf8().constData());
}

int main(int argc, char *argv[]) 
{
    // Install custom message handler before creating QApplication
    qInstallMessageHandler(customMessageHandler);

    // Create application with command-line argument support
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName("Codebase Processor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Codebase Tools");
    app.setOrganizationDomain("kgromero.com");

    // Set up application-wide styling (optional)
   // app.setStyle("Fusion");  // Modern, cross-platform look

    // Set window icon 
    QIcon appIcon(":/app_icon.png");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    } else {
        qWarning() << "Application icon could not be loaded";
    }

    // Exception handling for main window creation
    try {
        // Create main window
        MainWindow window;
        window.show();

        // Run event loop and capture exit code
        int exitCode = app.exec();

        // Log application exit

        return exitCode;
    } 
    catch (const std::exception& e) {
        // Handle any standard exceptions
        qCritical() << "Unhandled standard exception:" << e.what();
        QMessageBox::critical(nullptr, "Critical Error", 
            QString("An unexpected error occurred: %1\n\n"
                    "The application will now close.").arg(e.what()));
        return -1;
    } 
    catch (...) {
        // Handle any other unexpected exceptions
        qCritical() << "Unknown fatal error occurred";
        QMessageBox::critical(nullptr, "Critical Error", 
            "An unknown fatal error occurred. The application will now close.");
        return -2;
    }
}