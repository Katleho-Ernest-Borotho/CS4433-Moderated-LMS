#pragma once
#include "lms/application/IAuthenticationService.h"
#include "lms/application/AdminReportingService.h"
#include <QWidget>
#include <optional>
#include <string>
class QTableWidget;
class QLabel;
namespace lms {
class MaintenancePanel final : public QWidget {
public:
    MaintenancePanel(const IAuthenticationService& authentication,
        AdminReportingService& reporting,sqlite3* database,
        const std::optional<Session>& session,QString dataDirectory,QWidget* parent=nullptr);
    void refresh();
private:
    void backup();
    void exportCsv();
    void verifyBackup();
    const Session& administrator() const;
    const IAuthenticationService& auth_;
    AdminReportingService& reporting_;
    sqlite3* database_;
    const std::optional<Session>& session_;
    QString dataDirectory_;
    QTableWidget* metrics_=nullptr;
    QTableWidget* courses_=nullptr;
    QLabel* message_=nullptr;
};
} // namespace lms
