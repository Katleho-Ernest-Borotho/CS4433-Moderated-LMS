#pragma once

#include "lms/application/IAcademicManagementService.h"
#include "lms/application/ISubmissionService.h"

#include <QWidget>
#include <exception>
#include <optional>
#include <vector>

class QComboBox;
class QDateTimeEdit;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTextEdit;
class QPushButton;

namespace lms {

// Student: read-only assignments and personal effective deadlines.
// Lecturer: create assignments and grant extensions for assigned courses.
class AssignmentPanel final : public QWidget {
public:
    AssignmentPanel(IAcademicManagementService& academic,
                    ISubmissionService& submissions,
                    const std::optional<Session>& session,
                    QWidget* parent=nullptr);
    void refresh();

private:
    const Session& actor() const;
    void loadCourse();
    void createAssignment();
    void grantExtension();
    void refreshEffectiveDeadline();
    void showError(const std::exception& error);
    void chooseFile();
    void submitWork();
    void refreshHistory();
    void showSelectedVersion();

    IAcademicManagementService& academic_;
    ISubmissionService& submissions_;
    const std::optional<Session>& session_;

    QComboBox* courses_ = nullptr;
    QTableWidget* assignments_ = nullptr;
    QGroupBox* creationGroup_ = nullptr;
    QGroupBox* extensionGroup_ = nullptr;
    QLineEdit* assignmentTitle_ = nullptr;
    QDateTimeEdit* assignmentDeadline_ = nullptr;
    QDoubleSpinBox* maximumMark_ = nullptr;
    QComboBox* extensionAssignments_ = nullptr;
    QComboBox* enrolledStudents_ = nullptr;
    QDateTimeEdit* extensionDeadline_ = nullptr;
    QLabel* currentDeadline_ = nullptr;
    QGroupBox* studentSubmissionGroup_ = nullptr;
    QComboBox* submissionAssignments_ = nullptr;
    QTextEdit* submissionText_ = nullptr;
    QLineEdit* attachmentPath_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QTableWidget* versionHistory_ = nullptr;
    QTextEdit* savedText_ = nullptr;
    std::vector<SubmissionVersion> savedVersions_;
};

} // namespace lms
