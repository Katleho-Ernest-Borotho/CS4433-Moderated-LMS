#include "lms/persistence/SqliteDatabase.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <stdexcept>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("ModeratedLMS");
    application.setOrganizationName("CS4433");

    try {
        const QString dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory)) {
            throw std::runtime_error("Cannot create the application data folder");
        }

        lms::SqliteDatabase database((dataDirectory + "/lms.sqlite3").toStdString());
        const QString schemaFile = QCoreApplication::applicationDirPath() + "/lms_schema.sql";
        database.initializeSchemaFromFile(schemaFile.toStdString());

        QMainWindow window;
        window.setWindowTitle("Moderated Learning Management System");
        auto* central = new QWidget(&window);
        auto* layout = new QVBoxLayout(central);
        auto* title = new QLabel("Moderated Learning Management System", central);
        title->setAlignment(Qt::AlignCenter);
        auto* status = new QLabel("Local database is ready", central);
        status->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);
        layout->addWidget(status);
        window.setCentralWidget(central);
        window.resize(650, 350);
        window.show();
        return application.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, "LMS startup error", error.what());
        return 1;
    }
}
