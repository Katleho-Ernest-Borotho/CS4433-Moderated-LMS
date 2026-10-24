#pragma once

#include "lms/application/BootstrapService.h"
#include "lms/application/IAuthenticationService.h"

#include <QMainWindow>
#include <optional>

class QLabel;
class QLineEdit;
class QStackedWidget;
class QWidget;

namespace lms {
class LoginWindow final : public QMainWindow {
public:
    LoginWindow(IAuthenticationService& authentication, BootstrapService& bootstrap);

private:
    QWidget* createSetupPage();
    QWidget* createLoginPage();
    QWidget* createDashboardPage();
    void setupAdministrator();
    void attemptLogin();
    void showDashboard();

    IAuthenticationService& authentication_;
    BootstrapService& bootstrap_;
    QStackedWidget* pages_ = nullptr;
    QWidget* setupPage_ = nullptr;
    QWidget* loginPage_ = nullptr;
    QWidget* dashboardPage_ = nullptr;
    QLineEdit* setupUsername_ = nullptr;
    QLineEdit* setupPassword_ = nullptr;
    QLineEdit* setupConfirmation_ = nullptr;
    QLineEdit* loginUsername_ = nullptr;
    QLineEdit* loginPassword_ = nullptr;
    QLabel* dashboardGreeting_ = nullptr;
    std::optional<Session> session_;
};
} // namespace lms
