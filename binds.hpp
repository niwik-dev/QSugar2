#include <QObject>
#include <QMetaObject>
#include <QMetaMethod>
#include <QVariant>
#include <QEvent>

class RefBase : public QObject {
Q_OBJECT
signals:
    void valueChanged(const QVariant &newValue);

protected:
    RefBase(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    Q_INVOKABLE
    virtual void updateValue(QVariant value) =0;
};


class RefListener: public QObject{
Q_OBJECT
private:
    QString m_propertyName;
public:
    RefListener(QObject*parent= nullptr): QObject(parent){}

    void setPropertyName(QString propertyName){
        m_propertyName = std::move(propertyName);
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if(event->type() & QEvent::Type::InputMethod){
            emit valueChanged(watched->property(m_propertyName.toStdString().c_str()));
        }
        return QObject::eventFilter(watched, event);
    }
signals:
    void valueChanged(const QVariant &value);
};

template<typename T>
class Ref : public RefBase {
private:
    T value_;
    RefListener* listener_ = nullptr;
public:
    Ref(const Ref &other) : RefBase(other.parent()), value_(other.value()) {}
    Ref(T initialValue = T(), QObject *parent = nullptr) : RefBase(parent), value_(std::move(initialValue)) {}

    void setValue(T value) {
        if (value_ != value) {
            value_ = value;
            emit valueChanged(QVariant::fromValue(value));
        }
    }

    void updateValue(QVariant value) override {
        setValue(value.value<T>());
    }

    Ref& operator =(T value){
        this->setValue(value);
        return *this;
    }

    T value() const {
        return value_;
    }

    operator T() const {
        return value_;
    }

    void listenOn(QObject&target,QString propertyName){
        listenOn(&target, std::move(propertyName));
    }

    void listenOn(QObject*target,QString propertyName){
        if(listener_ == nullptr){
            listener_ = new RefListener(this);
            assert(listener_ != nullptr);
        }
        listener_->setPropertyName(std::move(propertyName));
        QObject::connect(listener_, &RefListener::valueChanged, [this](const QVariant &value) {
            this->setValue(value.value<T>());
        });
        target->installEventFilter(listener_);
    }

    void onChanged(const std::function<void(const T&)>& callback){
        QObject::connect(this, &RefBase::valueChanged,[callback](const QVariant& value) {
            callback(value.value<T>());
        });
    }
};

class ComputedBase : public QObject {
Q_OBJECT
signals:
    void updated(const QVariant& value);

protected:
    ComputedBase(QObject *parent = nullptr) : QObject(parent) {}
};

template<typename ReturnType>
class Computed: public ComputedBase{
private:
    std::function<ReturnType()> method_;
public:
    explicit Computed()= default;

    template<typename... Args>
    explicit Computed(Args&... args){
        addRef(args...);
    }

    template<typename T, typename... Args>
    void addRef(T& refObj, Args&... args) {
        refObj.onChanged([this](const QVariant &value) {
            emit this->updated(value);
        });
        addRef(args...);
    }

    template<typename T>
    void addRef(T& refObj) {
        refObj.onChanged([this](const QVariant &value) {
            emit this->updated(value);
        });
    }

    template<typename T>
    Computed<ReturnType>& operator << (Ref<T>& refObj) {
        this->addRef(refObj);
        return *this;
    }

    void onChanged(const std::function<void(const QVariant&)>& callback){
        QObject::connect(this, &ComputedBase::updated, callback);
    }

    void setMethod(std::function<ReturnType()> method) {
        method_ = std::move(method);
    }

    ReturnType operator ()() const {
        return method_();
    }
};

template<typename T>
void bind(QObject& target,const char* propertyName,Ref<T>& refProp){
    bind(&target, propertyName, refProp);
}

template<typename T>
void bind(QObject* target,const char* propertyName,Ref<T>& refProp){
    const QMetaObject *targetMetaObject = target->metaObject();
    do{
        int propertyIndex = targetMetaObject->indexOfProperty(propertyName);
        if(propertyIndex != -1){
            target->setProperty(propertyName, refProp.value());
            refProp.onChanged([target, propertyName](const T &value) {
                target->setProperty(propertyName, value);
            });
            break;
        }
        int methodIndex = targetMetaObject->indexOfMethod(QMetaObject::normalizedSignature(propertyName));
        if(methodIndex != -1){
            QMetaMethod method = targetMetaObject->method(methodIndex);
            method.invoke(target,Q_ARG(T, refProp.value()));
            refProp.onChanged([target, &method](const T &value) {
                method.invoke(target, Q_ARG(T, value));
            });
            break;
        }
    }while(false);
}

template<typename T>
void bind(QObject& target,const char* propertyName,Computed<T>& computedProp){
    bind(&target, propertyName, computedProp);
}

template<typename T>
void bind(QObject* target,const char* propertyName,Computed<T>& computedProp){
    const QMetaObject *metaObject = target->metaObject();
    do{
        int propertyIndex = metaObject->indexOfProperty(propertyName);
        if(propertyIndex != -1){
            target->setProperty(propertyName, computedProp());
            computedProp.onChanged([target, propertyName, &computedProp](const QVariant &value) {
                target->setProperty(propertyName, computedProp());
            });
            break;
        }
        int methodIndex = metaObject->indexOfMethod(QMetaObject::normalizedSignature(propertyName));
        if(methodIndex != -1){
            QMetaMethod method = metaObject->method(methodIndex);
            method.invoke(target,Q_ARG(T, computedProp()));
            computedProp.onChanged([target, &computedProp, &method](const QVariant &value) {
                method.invoke(target, Q_ARG(T, computedProp()));
            });
            break;
        }
    } while (false);
}

#define BIND(target, propertyName, prop) \
    bind(target, #propertyName, prop); \

#define BIND2(target, propertyName, prop) \
    bind(target, #propertyName, prop); \
    prop.listenOn(target, #propertyName);