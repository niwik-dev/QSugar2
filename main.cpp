#include <QCoreApplication>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include "binds.hpp"

class Model{
public:
    Ref<int> valueA;
    Ref<int> valueB;
    Computed<QString> message;
    Ref<QString> display;

    Model(){
        message << valueA << valueB;
        message.setMethod([this]{
            return QString("%1 + %2 = %3")
                    .arg(valueA)
                    .arg(valueB)
                    .arg(valueA+valueB);
        });
    }
};

class TestWidget:public QWidget{
    Q_OBJECT
private:
    QLabel* label;
    QSlider* sliderA;
    QSlider* sliderB;
public:
    explicit TestWidget(QWidget *parent = nullptr):QWidget(parent){
        QLayout* layout = new QVBoxLayout(this);

        label = new QLabel(this);
        layout->addWidget(label);

        sliderA = new QSlider(this);
        sliderA->setOrientation(Qt::Orientation::Horizontal);
        layout->addWidget(sliderA);

        sliderB = new QSlider(this);
        sliderB->setOrientation(Qt::Orientation::Horizontal);
        layout->addWidget(sliderB);
    }

    void bindOn(Model& model){
        BIND2(sliderA, value, model.valueA)
        BIND2(sliderB, value, model.valueB)

        BIND(label, text, model.message)
        BIND(label, text, model.display);
    }
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Model model;
    model.display = "No Data";

    TestWidget*widget = new TestWidget();
    widget->bindOn(model);
    widget->show();

    return QCoreApplication::exec();
}

#include "main.moc"