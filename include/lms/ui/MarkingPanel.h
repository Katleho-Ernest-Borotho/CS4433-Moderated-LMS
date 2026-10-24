#pragma once

#include "lms/application/IAcademicManagementService.h"
#include "lms/application/IMarkingService.h"
#include <QWidget>
#include <exception>
#include <optional>
#include <vector>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QTableWidget;
class QPushButton;

namespace lms {
// Lecturer-only mark and feedback interface. Service checks authorisation.
class MarkingPanel final : public QWidget {
public:
    MarkingPanel(IAcademicManagementService& academic, IMarkingService& marking,
                 const std::optional<Session>& session, QWidget* parent=nullptr);
    void refresh();
private:
    const Session& actor() const;
    void loadAssignments();
    void loadSubmissions();
    void selectSubmission();
    void saveMark();
    void saveAttachment();
    void showError(const std::exception& error);

    IAcademicManagementService& academic_;
    IMarkingService& marking_;
    const std::optional<Session>& session_;
    QComboBox* courses_=nullptr;
    QComboBox* assignments_=nullptr;
    QTableWidget* submissions_=nullptr;
    QPlainTextEdit* submittedText_=nullptr;
    QPlainTextEdit* feedback_=nullptr;
    QPlainTextEdit* revisionReason_=nullptr;
    QDoubleSpinBox* earnedMark_=nullptr;
    QLabel* status_=nullptr;
    QLabel* penalty_=nullptr;
    QPushButton* download_=nullptr;
    std::vector<MarkableSubmission> rows_;
    double assignmentMaximum_=100.0;
};
} // namespace lms
