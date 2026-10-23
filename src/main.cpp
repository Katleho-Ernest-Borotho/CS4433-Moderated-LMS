#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("Moderated LMS");

    QMainWindow window;
    window.setWindowTitle("Moderated Learning Management System");
    auto* central = new QWidget(&window);
    auto* layout = new QVBoxLayout(central);
    auto* title = new QLabel("Moderated Learning Management System", central);
    title->setAlignment(Qt::AlignCenter);
    auto* status = new QLabel("Initial implementation build", central);
    status->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(status);
    window.setCentralWidget(central);
    window.resize(650, 350);
    window.show();
    return application.exec();
}
