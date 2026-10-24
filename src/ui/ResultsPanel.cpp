#include "lms/ui/ResultsPanel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cstddef>
#include <exception>
#include <stdexcept>

namespace lms {
namespace {
QTableWidget* makeTable(QWidget* owner,const QStringList& columns)
{
    auto* table=new QTableWidget(owner);
    table->setColumnCount(columns.size());
    table->setHorizontalHeaderLabels(columns);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setMinimumHeight(150);
    return table;
}
QString localTime(const std::string& utc)
{
    const auto time=QDateTime::fromString(QString::fromStdString(utc),Qt::ISODate);
    return time.isValid()?QLocale(QLocale::English).toString(
        time.toLocalTime(),"dd MMM yyyy, HH:mm"):"Unknown";
}
} // namespace

ResultsPanel::ResultsPanel(IModerationService& moderation,
                           const std::optional<Session>& session,QWidget* parent)
    : QWidget(parent),moderation_(moderation),session_(session)
{
    auto* outer=new QVBoxLayout(this);
    auto* scroll=new QScrollArea(this);
    scroll->setWidgetResizable(true);
    outer->addWidget(scroll);
    auto* body=new QWidget(scroll);
    scroll->setWidget(body);
    auto* layout=new QVBoxLayout(body);

    hodSection_=new QGroupBox("Departmental moderation and result release",body);
    auto* moderationLayout=new QVBoxLayout(hodSection_);
    auto* intro=new QLabel(
        "Review the latest submission and provisional mark. Approve the result "
        "before releasing it. Results cannot be approved or released until "
        "the student's effective deadline has passed.",hodSection_);
    intro->setWordWrap(true);
    moderationLayout->addWidget(intro);
    moderationTable_=makeTable(hodSection_,
        {"Course","Assignment","Student","Earned","Penalty","Final","Status"});
    moderationLayout->addWidget(moderationTable_);

    auto* review=new QFormLayout;
    submittedText_=new QPlainTextEdit(hodSection_);
    submittedText_->setReadOnly(true);
    submittedText_->setMaximumHeight(95);
    feedback_=new QPlainTextEdit(hodSection_);
    feedback_->setReadOnly(true);
    feedback_->setMaximumHeight(85);
    deadline_=new QLabel(hodSection_);
    deadline_->setWordWrap(true);
    breakdown_=new QLabel(hodSection_);
    breakdown_->setWordWrap(true);
    attachmentButton_=new QPushButton("Save latest attached file...",hodSection_);
    attachmentButton_->setEnabled(false);
    adjust_=new QCheckBox("Adjust lecturer's earned mark",hodSection_);
    earned_=new QDoubleSpinBox(hodSection_);
    earned_->setRange(0,1000000);
    earned_->setDecimals(2);
    earned_->setEnabled(false);
    reason_=new QPlainTextEdit(hodSection_);
    reason_->setPlaceholderText("Required if changing the mark");
    reason_->setMaximumHeight(60);
    review->addRow("Latest submission:",submittedText_);
    review->addRow("Attachment:",attachmentButton_);
    review->addRow("Lecturer feedback:",feedback_);
    review->addRow("Effective deadline:",deadline_);
    review->addRow("Assessment:",breakdown_);
    review->addRow(adjust_);
    review->addRow("Adjusted earned mark:",earned_);
    review->addRow("Adjustment reason:",reason_);
    moderationLayout->addLayout(review);

    auto* approveButton=new QPushButton("Approve moderation",hodSection_);
    auto* releaseButton=new QPushButton("Release approved result",hodSection_);
    auto* refreshModeration=new QPushButton("Refresh departmental results",hodSection_);
    moderationLayout->addWidget(approveButton);
    moderationLayout->addWidget(releaseButton);
    moderationLayout->addWidget(refreshModeration);
    layout->addWidget(hodSection_);

    studentSection_=new QGroupBox("My released results",body);
    auto* studentLayout=new QVBoxLayout(studentSection_);
    auto* notice=new QLabel(
        "Only approved, officially released marks and lecturer feedback appear here.",
        studentSection_);
    notice->setWordWrap(true);
    studentLayout->addWidget(notice);
    releasedTable_=makeTable(studentSection_,
        {"Course","Assignment","Earned","Penalty","Final / maximum","Released (local)"});
    studentLayout->addWidget(releasedTable_);
    releasedFeedback_=new QLabel("Choose a released result to read feedback.",studentSection_);
    releasedFeedback_->setWordWrap(true);
    studentLayout->addWidget(releasedFeedback_);
    auto* refreshStudent=new QPushButton("Refresh released results",studentSection_);
    studentLayout->addWidget(refreshStudent);
    layout->addWidget(studentSection_);
    layout->addStretch();

    connect(moderationTable_,&QTableWidget::itemSelectionChanged,this,
            [this]{chooseModeration();});
    connect(releasedTable_,&QTableWidget::itemSelectionChanged,this,
            [this]{chooseStudentResult();});
    connect(adjust_,&QCheckBox::toggled,earned_,&QDoubleSpinBox::setEnabled);
    connect(approveButton,&QPushButton::clicked,this,[this]{approve();});
    connect(releaseButton,&QPushButton::clicked,this,[this]{release();});
    connect(attachmentButton_,&QPushButton::clicked,this,[this]{saveAttachment();});
    connect(refreshModeration,&QPushButton::clicked,this,[this]{refresh();});
    connect(refreshStudent,&QPushButton::clicked,this,[this]{refresh();});
}

const Session& ResultsPanel::actor() const
{
    if(!session_)throw AuthenticationError("No active session");
    return *session_;
}
void ResultsPanel::showError(const std::exception& error)
{
    QMessageBox::warning(this,"Results",QString::fromUtf8(error.what()));
}

void ResultsPanel::refresh()
{
    items_.clear();
    released_.clear();
    moderationTable_->setRowCount(0);
    releasedTable_->setRowCount(0);
    submittedText_->clear();
    feedback_->clear();
    reason_->clear();
    releasedFeedback_->setText("Choose a released result to read feedback.");
    deadline_->clear();
    breakdown_->clear();
    attachmentButton_->setEnabled(false);
    try {
        const auto role=actor().role();
        hodSection_->setVisible(role==Role::HeadOfDepartment);
        studentSection_->setVisible(role==Role::Student);
        if(role==Role::HeadOfDepartment) {
            items_=moderation_.listDepartmentResults(actor());
            moderationTable_->setRowCount(static_cast<int>(items_.size()));
            for(int i=0;i<static_cast<int>(items_.size());++i) {
                const auto& item=items_[static_cast<std::size_t>(i)];
                const QString cells[]={
                    QString::fromStdString(item.courseCode),
                    QString::fromStdString(item.assignmentTitle),
                    QString::fromStdString(item.studentUsername),
                    QString::number(item.earnedMark,'f',2),
                    QString::number(item.penaltyMark,'f',2),
                    QString::number(item.finalMark,'f',2),
                    item.markedVersionId!=item.latestVersionId ? "Needs re-mark" :
                    item.approved?"Approved":"Provisional"};
                for(int col=0;col<7;++col)
                    moderationTable_->setItem(i,col,new QTableWidgetItem(cells[col]));
            }
        } else if(role==Role::Student) {
            released_=moderation_.getReleasedResults(actor());
            releasedTable_->setRowCount(static_cast<int>(released_.size()));
            for(int i=0;i<static_cast<int>(released_.size());++i) {
                const auto& item=released_[static_cast<std::size_t>(i)];
                const QString cells[]={
                    QString::fromStdString(item.courseCode),
                    QString::fromStdString(item.assignmentTitle),
                    QString::number(item.earnedMark,'f',2),
                    QString::number(item.penaltyMark,'f',2),
                    QString("%1 / %2").arg(item.finalMark,0,'f',2)
                                         .arg(item.maximumMark,0,'f',2),
                    localTime(item.releasedAtUtc)};
                for(int col=0;col<6;++col)
                    releasedTable_->setItem(i,col,new QTableWidgetItem(cells[col]));
            }
        }
    } catch(const std::exception& error){showError(error);}
}

void ResultsPanel::chooseModeration()
{
    const int i=moderationTable_->currentRow();
    if(i<0||i>=static_cast<int>(items_.size()))return;
    const auto& item=items_[static_cast<std::size_t>(i)];
    submittedText_->setPlainText(item.submittedText.empty()
        ?QString("(File-only submission)") :QString::fromStdString(item.submittedText));
    feedback_->setPlainText(QString::fromStdString(item.feedback));
    deadline_->setText(localTime(item.effectiveDeadlineUtc)+
        " (after this time moderation and release are permitted)");
    breakdown_->setText(QString("Earned %1  •  Late penalty %2  •  Final %3 / %4")
        .arg(item.earnedMark,0,'f',2).arg(item.penaltyMark,0,'f',2)
        .arg(item.finalMark,0,'f',2).arg(item.maximumMark,0,'f',2));
    earned_->setMaximum(item.maximumMark);
    earned_->setValue(item.earnedMark);
    adjust_->setChecked(false);
    reason_->clear();
    attachmentButton_->setEnabled(!item.attachmentRef.empty());
}

void ResultsPanel::approve()
{
    const int i=moderationTable_->currentRow();
    if(i<0||i>=static_cast<int>(items_.size())) {
        QMessageBox::information(this,"Select assessment","Select a departmental result first.");
        return;
    }
    try {
        const auto& item=items_[static_cast<std::size_t>(i)];
        if(QMessageBox::question(this,"Approve result",
            QString("Approve the result for %1? This is separate from release.")
                 .arg(QString::fromStdString(item.studentUsername)))!=QMessageBox::Yes) return;
        moderation_.moderate(actor(),item.submissionId,
            adjust_->isChecked()?std::optional<double>(earned_->value()):std::nullopt,
            reason_->toPlainText().toStdString());
        refresh();
        QMessageBox::information(this,"Moderation saved",
            "The result is approved but still unreleased.");
    } catch(const std::exception& error){showError(error);}
}

void ResultsPanel::release()
{
    const int i=moderationTable_->currentRow();
    if(i<0||i>=static_cast<int>(items_.size())) {
        QMessageBox::information(this,"Select assessment","Select an approved result first.");
        return;
    }
    try {
        const auto& item=items_[static_cast<std::size_t>(i)];
        if(QMessageBox::question(this,"Release result",
            QString("Release the approved result to %1? The Student will then see the mark and feedback.")
                .arg(QString::fromStdString(item.studentUsername)))!=QMessageBox::Yes) return;
        moderation_.release(actor(),item.submissionId);
        refresh();
        QMessageBox::information(this,"Result released",
            "The Student can now view the final mark and feedback.");
    } catch(const std::exception& error){showError(error);}
}

void ResultsPanel::saveAttachment()
{
    const int i=moderationTable_->currentRow();
    if(i<0||i>=static_cast<int>(items_.size()))return;
    try {
        const auto& item=items_[static_cast<std::size_t>(i)];
        const auto dot=item.attachmentRef.find_last_of('.');
        const std::string extension=dot==std::string::npos?"":item.attachmentRef.substr(dot);
        const auto target=QFileDialog::getSaveFileName(
            this,"Save latest submission",QString("submission")+QString::fromStdString(extension));
        if(target.isEmpty())return;
        const std::string bytes=moderation_.readLatestAttachment(actor(),item.submissionId);
        QFile file(target);
        if(!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Cannot save attachment at that location");
        if(file.write(bytes.data(),static_cast<qint64>(bytes.size()))
           !=static_cast<qint64>(bytes.size())) {
            file.close();
            file.remove();
            throw std::runtime_error("Attachment could not be written completely");
        }
        file.close();
        QMessageBox::information(this,"Attachment saved","The file has been saved.");
    } catch(const std::exception& error){showError(error);}
}

void ResultsPanel::chooseStudentResult()
{
    const int i=releasedTable_->currentRow();
    if(i<0||i>=static_cast<int>(released_.size()))return;
    releasedFeedback_->setText("Feedback: "+
        QString::fromStdString(released_[static_cast<std::size_t>(i)].feedback));
}
} // namespace lms
