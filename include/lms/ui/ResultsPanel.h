#pragma once

#include "lms/application/IModerationService.h"
#include <QWidget>
#include <exception>
#include <optional>
#include <vector>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QGroupBox;

namespace lms {
// HoD: review -> approve/adjust -> release. Student: released results only.
class ResultsPanel final : public QWidget {
public:
    ResultsPanel(IModerationService& moderation,const std::optional<Session>& session,
                 QWidget* parent=nullptr);
    void refresh();
private:
    const Session& actor() const;
    void chooseModeration();
    void approve();
    void release();
    void saveAttachment();
    void chooseStudentResult();
    void showError(const std::exception& error);

    IModerationService& moderation_;
    const std::optional<Session>& session_;
    QGroupBox* hodSection_=nullptr;
    QGroupBox* studentSection_=nullptr;
    QTableWidget* moderationTable_=nullptr;
    QTableWidget* releasedTable_=nullptr;
    QPlainTextEdit* submittedText_=nullptr;
    QPlainTextEdit* feedback_=nullptr;
    QPlainTextEdit* reason_=nullptr;
    QCheckBox* adjust_=nullptr;
    QDoubleSpinBox* earned_=nullptr;
    QLabel* breakdown_=nullptr;
    QLabel* deadline_=nullptr;
    QLabel* releasedFeedback_=nullptr;
    QPushButton* attachmentButton_=nullptr;
    std::vector<ModerationRecord> items_;
    std::vector<ReleasedResult> released_;
};
} // namespace lms
