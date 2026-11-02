#pragma once

#include "lms/application/BootstrapService.h"
#include "lms/application/IAuthenticationService.h"
#include "lms/application/IAcademicManagementService.h"
#include "lms/application/ISubmissionService.h"
#include "lms/application/IMarkingService.h"
#include "lms/application/IModerationService.h"
#include "lms/application/AdminReportingService.h"

#include <QMainWindow>
#include <QString>
#include <optional>

class QLabel;
class QLineEdit;
class QComboBox;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QWidget;

namespace lms {
class AcademicPanel;
class AssignmentPanel;
class MarkingPanel;
class ResultsPanel;
class MaintenancePanel;
class LoginWindow final : public QMainWindow {
public:
    LoginWindow(IAuthenticationService& authentication, BootstrapService& bootstrap,
                IAcademicManagementService& accounts, ISubmissionService& submissions,
                IMarkingService& marking, IModerationService& moderation,
                AdminReportingService& reporting, sqlite3* database, QString dataDirectory);

private:
    QWidget* createSetupPage();
    QWidget* createLoginPage();
    QWidget* createDashboardPage();
    QWidget* createAdminPanel();
    void setupAdministrator();
    void attemptLogin();
    void showDashboard();
    void createAccount();
    void refreshAccounts();
    void deactivateAccount();
    void renameAccount();
    void reactivateAccount();
    void resetAccountPassword();
    void logOut();

    IAuthenticationService& authentication_;
    BootstrapService& bootstrap_;
    IAcademicManagementService& accounts_;
    ISubmissionService& submissions_;
    IMarkingService& marking_;
    IModerationService& moderation_;
    AdminReportingService& reporting_;
    sqlite3* database_ = nullptr;
    QString dataDirectory_;
    QStackedWidget* pages_ = nullptr;
    QWidget* setupPage_ = nullptr;
    QWidget* loginPage_ = nullptr;
    QWidget* dashboardPage_ = nullptr;
    QWidget* adminPanel_ = nullptr;
    AcademicPanel* academicPanel_ = nullptr;
    QTabWidget* adminTabs_ = nullptr;
    QTabWidget* learningTabs_ = nullptr;
    AssignmentPanel* assignmentsPanel_ = nullptr;
    MarkingPanel* markingPanel_ = nullptr;
    ResultsPanel* resultsPanel_ = nullptr;
    MaintenancePanel* maintenancePanel_ = nullptr;
    QTableWidget* myCoursesTable_ = nullptr;
    QLineEdit* setupUsername_ = nullptr;
    QLineEdit* setupPassword_ = nullptr;
    QLineEdit* setupConfirmation_ = nullptr;
    QLineEdit* loginUsername_ = nullptr;
    QLineEdit* loginPassword_ = nullptr;
    QLineEdit* newUsername_ = nullptr;
    QLineEdit* newPassword_ = nullptr;
    QLineEdit* newPasswordConfirmation_ = nullptr;
    QComboBox* newRole_ = nullptr;
    QTableWidget* accountTable_ = nullptr;
    QLabel* dashboardGreeting_ = nullptr;
    std::optional<Session> session_;
};
} // namespace lms
