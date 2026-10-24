#include "lms/application/AcademicManagementService.h"
#include "lms/application/AuthenticationService.h"
#include "lms/application/BootstrapService.h"
#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"
#include "lms/ui/LoginWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include <exception>
#include <stdexcept>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("ModeratedLMS");
    application.setOrganizationName("CS4433");

    try {
        const QString dataDirectory =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory)) {
            throw std::runtime_error("Cannot create the application data folder");
        }

        lms::SqliteDatabase database((dataDirectory + "/lms.sqlite3").toStdString());
        const QString schemaFile = QCoreApplication::applicationDirPath() + "/lms_schema.sql";
        database.initializeSchemaFromFile(schemaFile.toStdString());

        lms::SQLiteRecordRepository users(database);
        lms::SodiumPasswordHasher passwords;
        lms::AuthenticationService authentication(users, passwords);
        lms::BootstrapService bootstrap(users, passwords);
        lms::AcademicManagementService academic(authentication, users, users, passwords);

        lms::LoginWindow window(authentication, bootstrap, academic);
        window.show();
        return application.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, "LMS startup error", error.what());
        return 1;
    }
}
