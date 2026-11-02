// Local development recovery utility. Deliberately excluded from normal builds.
// Requires access to the Windows user's local application data and a closed LMS.
#include "lms/application/AccountValidation.h"
#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStringList>
#include <QVBoxLayout>

#include <sqlite3.h>
#include <cstdint>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
struct AdminAccount {
    std::int64_t id=0;
    QString username;
    bool active=false;
};

QString createBackup(sqlite3* source,const QString& databasePath)
{
    const QString backupPath=databasePath+
        ".before-admin-recovery-"+QDateTime::currentDateTimeUtc()
              .toString("yyyyMMdd_HHmmss_zzz")+".bak";
    sqlite3* destination=nullptr;
    const QByteArray path=QDir::toNativeSeparators(backupPath).toUtf8();
    if(sqlite3_open_v2(path.constData(),&destination,
                       SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,nullptr)!=SQLITE_OK) {
        const std::string error=destination?sqlite3_errmsg(destination):
            "Cannot open database backup file";
        if(destination)sqlite3_close(destination);
        throw std::runtime_error(error);
    }
    sqlite3_backup* backup=sqlite3_backup_init(destination,"main",source,"main");
    if(!backup) {
        const std::string error=sqlite3_errmsg(destination);
        sqlite3_close(destination);
        QFile::remove(backupPath);
        throw std::runtime_error("Backup could not start: "+error);
    }
    const int step=sqlite3_backup_step(backup,-1);
    const int finish=sqlite3_backup_finish(backup);
    const int finalStatus=sqlite3_errcode(destination);
    sqlite3_close(destination);
    if(step!=SQLITE_DONE || finish!=SQLITE_OK || finalStatus!=SQLITE_OK) {
        QFile::remove(backupPath);
        throw std::runtime_error("Backup failed; password was NOT changed");
    }
    return backupPath;
}

bool collectPassword(QWidget* parent,QString& password)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Choose a new Administrator password");
    auto* layout=new QVBoxLayout(&dialog);
    auto* details=new QLabel(
        "Choose a new password of at least 12 characters. "
        "It will replace the old password; existing accounts and academic data remain.",&dialog);
    details->setWordWrap(true);
    layout->addWidget(details);
    auto* fields=new QFormLayout;
    auto* first=new QLineEdit(&dialog);
    first->setEchoMode(QLineEdit::Password);
    auto* second=new QLineEdit(&dialog);
    second->setEchoMode(QLineEdit::Password);
    fields->addRow("New password:",first);
    fields->addRow("Confirm password:",second);
    layout->addLayout(fields);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return false;
    if(first->text()!=second->text()) {
        QMessageBox::warning(parent,"Password mismatch","The passwords do not match.");
        return false;
    }
    password=first->text();
    first->clear();
    second->clear();
    return true;
}
} // namespace

int main(int argc,char** argv)
{
    QApplication app(argc,argv);
    // MUST match main.cpp so QStandardPaths resolves the same local database.
    QCoreApplication::setOrganizationName("CS4433");
    QCoreApplication::setApplicationName("ModeratedLMS");
    try {
        const QString directory=QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation);
        const QString dbFile=directory+"/lms.sqlite3";
        if(!QFileInfo::exists(dbFile)) {
            QMessageBox::critical(nullptr,"Database not found",
                "No LMS database exists for this Windows account.\n"
                "Expected:\n"+QDir::toNativeSeparators(dbFile)+
                "\nDo not create or replace the database to recover a password.");
            return 1;
        }
        if(QMessageBox::question(nullptr,"Local Administrator recovery",
            "Use this utility only on your own development computer.\n\n"
            "Close the LMS first. This recovery tool can reset a local "
            "Administrator password without knowing the old password.\n"
            "A full SQLite backup will be created before any change.\n\n"
            "Continue?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)
            return 0;

        lms::SqliteDatabase database(dbFile.toStdString());
        lms::SQLiteRecordRepository users(database);
        std::vector<AdminAccount> administrators;
        QStringList options;
        for(const auto& user:users.listUsers()) {
            if(user.role!=lms::Role::Administrator)continue;
            administrators.push_back({user.id,QString::fromStdString(user.username),user.active});
            options<<QString("%1 (%2)").arg(administrators.back().username)
                                    .arg(user.active?"Active":"Inactive");
        }
        if(administrators.empty()) {
            QMessageBox::critical(nullptr,"No Administrator found",
                "No Administrator record exists. No changes were made.");
            return 1;
        }
        bool accepted=false;
        const QString selected=QInputDialog::getItem(nullptr,
            "Recover Administrator","Select the Administrator account:",
            options,0,false,&accepted);
        if(!accepted)return 0;
        const int index=options.indexOf(selected);
        if(index<0 || index>=static_cast<int>(administrators.size()))return 1;
        const AdminAccount account=administrators[static_cast<std::size_t>(index)];
        QString proposed;
        if(!collectPassword(nullptr,proposed))return 0;

        const std::string password=proposed.toStdString();
        proposed.fill(QChar('\0'));
        lms::validatePassword(password);
        lms::SodiumPasswordHasher hasher;
        const std::string hash=hasher.hash(password);

        if(QMessageBox::question(nullptr,"Confirm password reset",
            "Reset the password of Administrator '"+account.username+"'?\n"
            "This will invalidate previous sessions. The existing database "
            "will be backed up first.",QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No)!=QMessageBox::Yes)return 0;

        const QString backupPath=createBackup(database.connection(),dbFile);
        // Hash replacement uses a prepared statement inside a SQLite transaction.
        users.resetPasswordHash(account.id,hash);
        if(!account.active) {
            users.reactivateUser(account.id);
        }
        QMessageBox::information(nullptr,"Administrator recovered",
            "Administrator '"+account.username+"' has a new password.\n"
            "No academic records were deleted.\n\n"
            "Database backup:\n"+QDir::toNativeSeparators(backupPath)+
            "\n\nClose this utility, then launch the normal LMS and sign in.");
        return 0;
    } catch(const std::exception& e) {
        QMessageBox::critical(nullptr,"Administrator recovery failed",
            QString::fromUtf8(e.what())+
            "\nNo automatic database deletion or recreation was performed.");
        return 1;
    }
}
