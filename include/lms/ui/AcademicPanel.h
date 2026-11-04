#pragma once

#include "lms/application/IAcademicManagementService.h"
#include <QWidget>
#include <optional>

class QComboBox;
class QLineEdit;
class QTableWidget;

namespace lms {
// Admin-only presentation component; AcademicManagementService enforces permissions.
class AcademicPanel final : public QWidget {
public:
    AcademicPanel(IAcademicManagementService& service,
                  const std::optional<Session>& currentSession,
                  QWidget* parent = nullptr);
    void refresh();

private:
    const Session& actor() const;
    void createDepartment();
    void editDepartment();
    void createCourse();
    void editCourse();
    void assignHoD();
    void assignLecturer();
    void enrolStudent();
    void reportFailure(const std::exception& error);

    IAcademicManagementService& service_;
    const std::optional<Session>& session_;

    QLineEdit* departmentName_ = nullptr;
    QComboBox* departments_ = nullptr;
    QTableWidget* departmentsTable_ = nullptr;
    QLineEdit* courseCode_ = nullptr;
    QLineEdit* courseTitle_ = nullptr;
    QComboBox* courseDepartments_ = nullptr;
    QComboBox* courses_ = nullptr;
    QTableWidget* coursesTable_ = nullptr;
    QComboBox* hodUsers_ = nullptr;
    QComboBox* hodDepartments_ = nullptr;
    QComboBox* lecturerUsers_ = nullptr;
    QComboBox* lecturerCourses_ = nullptr;
    QComboBox* studentUsers_ = nullptr;
    QComboBox* studentCourses_ = nullptr;
};
} // namespace lms
