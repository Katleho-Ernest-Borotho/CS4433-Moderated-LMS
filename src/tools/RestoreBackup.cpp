#include "lms/persistence/BackupManager.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <filesystem>
#include <stdexcept>

int main(int argc,char** argv)
{
    QApplication app(argc,argv);
    QCoreApplication::setOrganizationName("CS4433");
    QCoreApplication::setApplicationName("ModeratedLMS");
    const QString data=QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if(data.isEmpty()||!QFileInfo::exists(data+"/lms.sqlite3")) {
        QMessageBox::critical(nullptr,"Restore unavailable",
            "Existing LMS database was not found. No files were changed.");
        return 1;
    }
    // Same lock as the main application; prevents restore while LMS is running.
    QLockFile lock(data+"/lms.lock");
    lock.setStaleLockTime(0);
    if(!lock.tryLock(0)) {
        QMessageBox::critical(nullptr,"Close the LMS first",
            "The LMS is open or another maintenance process is active. "
            "Close it before using Restore.");
        return 1;
    }
    try {
        if(QMessageBox::warning(nullptr,"Restore existing LMS data",
            "This replaces current accounts, courses, submissions and marks "
            "with the selected backup. An automatic safety backup is created "
            "before replacement. Continue?",QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No)!=QMessageBox::Yes)return 0;
        const QString selected=QFileDialog::getExistingDirectory(
            nullptr,"Choose a ModeratedLMS_Backup folder to restore");
        if(selected.isEmpty())return 0;
        const auto package=std::filesystem::path(selected.toStdString());
        const auto verified=lms::BackupManager::verify(package);
        bool accepted=false;
        const QString confirmation=QInputDialog::getText(
            nullptr,"Confirm database restore",
            QString("Backup verified: %1 attachments.\n"
                    "Type RESTORE to replace current LMS data:")
                .arg(verified.attachmentCount),
            QLineEdit::Normal,QString(),&accepted);
        if(!accepted || confirmation!="RESTORE")return 0;
        const auto safety=lms::BackupManager::restore(
            package,std::filesystem::path(data.toStdString()));
        QMessageBox::information(nullptr,"Restore completed",
            "The selected database and attachments have been restored.\n"
            "A backup of your PREVIOUS live data was retained here:\n"+
            QDir::toNativeSeparators(QString::fromStdString(safety.string()))+
            "\n\nClose this utility and start the normal LMS.");
        return 0;
    } catch(const std::exception& e) {
        QMessageBox::critical(nullptr,"Restore failed",QString::fromUtf8(e.what())+
            "\nThe original data was not intentionally deleted. Check the "
            "pre-restore safety backup before attempting recovery again.");
        return 1;
    }
}
