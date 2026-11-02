#include "lms/ui/MaintenancePanel.h"
#include "lms/persistence/BackupManager.h"
#include <QAbstractItemView>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <stdexcept>

namespace lms {
namespace {
QTableWidget* table(QWidget* parent,const QStringList& headings) {
    auto* t=new QTableWidget(parent);
    t->setColumnCount(headings.size());
    t->setHorizontalHeaderLabels(headings);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setMinimumHeight(170);
    return t;
}
void cell(QTableWidget* t,int row,int col,const QString& value) {
    t->setItem(row,col,new QTableWidgetItem(value));
}
}

MaintenancePanel::MaintenancePanel(
    const IAuthenticationService& authentication,AdminReportingService& reporting,
    sqlite3* database,const std::optional<Session>& session,QString dataDirectory,
    QWidget* parent)
    : QWidget(parent),auth_(authentication),reporting_(reporting),database_(database),
      session_(session),dataDirectory_(std::move(dataDirectory))
{
    auto* layout=new QVBoxLayout(this);
    auto* scroll=new QScrollArea(this);
    scroll->setWidgetResizable(true);
    layout->addWidget(scroll);
    auto* page=new QWidget(scroll);
    scroll->setWidget(page);
    auto* body=new QVBoxLayout(page);
    auto* heading=new QLabel("Maintenance, backup and reporting",page);
    heading->setStyleSheet("font-weight: bold; font-size: 18px");
    body->addWidget(heading);
    auto* warning=new QLabel(
        "Backups include password hashes, student work and results. "
        "Store them in a protected folder. They are not encrypted.",page);
    warning->setWordWrap(true);
    body->addWidget(warning);
    auto* makeBackup=new QPushButton("Create full backup...",page);
    auto* verify=new QPushButton("Verify backup folder...",page);
    auto* exportButton=new QPushButton("Export administrative CSV...",page);
    auto* refreshButton=new QPushButton("Refresh report",page);
    body->addWidget(makeBackup);
    body->addWidget(verify);
    body->addWidget(exportButton);
    body->addWidget(refreshButton);
    auto* restoreNotice=new QLabel(
        "Restore is deliberately offline. Close the LMS and start the separate "
        "ModeratedLMSRestore tool to restore a verified backup.",page);
    restoreNotice->setWordWrap(true);
    body->addWidget(restoreNotice);
    message_=new QLabel(page);
    message_->setWordWrap(true);
    body->addWidget(message_);
    metrics_=table(page,{"Metric","Count"});
    courses_=table(page,{"Course","Department","Enrolled","Assignments","Submissions","Released"});
    body->addWidget(metrics_);
    body->addWidget(courses_);
    body->addStretch();

    connect(makeBackup,&QPushButton::clicked,this,[this]{backup();});
    connect(verify,&QPushButton::clicked,this,[this]{verifyBackup();});
    connect(exportButton,&QPushButton::clicked,this,[this]{exportCsv();});
    connect(refreshButton,&QPushButton::clicked,this,[this]{refresh();});
}
const Session& MaintenancePanel::administrator() const {
    if(!session_)throw AuthenticationError("Administrator session missing");
    auth_.requireRole(*session_,Role::Administrator);
    return *session_;
}
void MaintenancePanel::refresh() {
    try {
        const auto r=reporting_.generate(administrator());
        const std::pair<QString,std::int64_t> rows[]={
            {"Active users",r.activeUsers},{"Inactive users",r.inactiveUsers},
            {"Departments",r.departments},{"Courses",r.courses},
            {"Assignments",r.assignments},{"Submissions",r.submissions},
            {"Submission versions",r.versions},{"Provisional results",r.provisional},
            {"Approved but unreleased",r.approvedUnreleased},{"Released results",r.released}};
        metrics_->setRowCount(10);
        for(int i=0;i<10;++i) {
            cell(metrics_,i,0,rows[i].first);
            cell(metrics_,i,1,QString::number(rows[i].second));
        }
        courses_->setRowCount(static_cast<int>(r.courseRows.size()));
        for(int i=0;i<static_cast<int>(r.courseRows.size());++i) {
            const auto& c=r.courseRows[static_cast<std::size_t>(i)];
            cell(courses_,i,0,QString::fromStdString(c.code));
            cell(courses_,i,1,QString::fromStdString(c.department));
            cell(courses_,i,2,QString::number(c.enrolled));
            cell(courses_,i,3,QString::number(c.assignments));
            cell(courses_,i,4,QString::number(c.submissions));
            cell(courses_,i,5,QString::number(c.releasedResults));
        }
    } catch(const std::exception& e) {
        QMessageBox::warning(this,"Reporting error",e.what());
    }
}
void MaintenancePanel::backup() {
    try {
        administrator();
        const auto parent=QFileDialog::getExistingDirectory(this,
            "Choose a protected location OUTSIDE LMS data for the backup");
        if(parent.isEmpty())return;
        if(QMessageBox::question(this,"Create full backup",
            "Create a backup containing all database records, passwords hashes and "
            "stored attachments in the selected folder? It will NOT be encrypted.")
            !=QMessageBox::Yes)return;
        const auto result=BackupManager::create(database_,
            std::filesystem::path(dataDirectory_.toStdString()),
            std::filesystem::path(parent.toStdString()));
        const auto folder=QString::fromStdString(result.folder.string());
        message_->setText("Verified backup: "+QDir::toNativeSeparators(folder));
        QMessageBox::information(this,"Backup completed",
            QString("Backup saved and verified.\nAttachments: %1\nFolder:\n%2")
                .arg(result.attachmentCount).arg(QDir::toNativeSeparators(folder)));
    } catch(const std::exception& e) {
        QMessageBox::critical(this,"Backup failed",e.what());
    }
}
void MaintenancePanel::verifyBackup() {
    try {
        administrator();
        const auto folder=QFileDialog::getExistingDirectory(this,"Choose LMS backup folder");
        if(folder.isEmpty())return;
        const auto result=BackupManager::verify(
            std::filesystem::path(folder.toStdString()));
        QMessageBox::information(this,"Backup verified",
            QString("Database and %1 stored attachments passed integrity checks.")
                .arg(result.attachmentCount));
    } catch(const std::exception& e) {
        QMessageBox::warning(this,"Backup verification failed",e.what());
    }
}
void MaintenancePanel::exportCsv() {
    try {
        const auto report=reporting_.generate(administrator());
        const auto path=QFileDialog::getSaveFileName(this,"Export summary",
            "ModeratedLMS_AdminReport.csv","CSV files (*.csv)");
        if(path.isEmpty())return;
        const auto text=AdminReportingService::toCsv(report);
        QFile file(path);
        if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
            throw std::runtime_error("Cannot write CSV at the selected location");
        const auto bytes=file.write(text.data(),static_cast<qint64>(text.size()));
        if(bytes!=static_cast<qint64>(text.size()))
            throw std::runtime_error("CSV could not be completely written");
        file.close();
        QMessageBox::information(this,"Report exported",
            "Administrative CSV saved. It contains aggregate counts but no passwords.");
    } catch(const std::exception& e) {
        QMessageBox::warning(this,"CSV export failed",e.what());
    }
}
} // namespace lms
