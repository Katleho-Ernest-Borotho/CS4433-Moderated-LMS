#pragma once

#include "lms/application/BootstrapService.h"
#include "lms/application/IAuthenticationService.h"
#include "lms/application/IAcademicManagementService.h"

#include <QMainWindow>
#include <optional>

class QLabel;
class QLineEdit;
class QComboBox;
class QStackedWidget;
class QTableWidget;
class QWidget;

namespace lms {
class LoginWindow final : public QMainWindow {
public:
    LoginWindow(IAuthenticationService& authentication, BootstrapService& bootstrap,
                IAcademicManagementService& accounts);

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
    void logOut();
    void displayAccountError(const std::exception& error);

    IAuthenticationService& authentication_;
    BootstrapService& bootstrap_;
    IAcademicManagementService& accounts_;
    QStackedWidget* pages_ = nullptr;
    QWidget* setupPage_ = nullptr;
    QWidget* loginPage_ = nullptr;
    QWidget* dashboardPage_ = nullptr;
    QWidget* adminPanel_ = nullptr;
    QLineEdit* setupUsername_ = nullptr;
    QLineEdit* setupPassword_ = nullptr;
    QLineEdit* setupConfirmation_ = nullptr;
    QLineEdit* loginUsername_ = nullptr;
    QLineEdit* loginPassword_ = nullptr;
    QLineEdit* newUsername_ = nullptr;
    QLineEdit* newPassword_ = nullptr;
    QComboBox* newRole_ = nullptr;
    QTableWidget* accountTable_ = nullptr;
    QLabel* dashboardGreeting_ = nullptr;
    std::optional<Session> session_;
};
} // namespace lms
