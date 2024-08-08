<link href='https://fonts.googleapis.com/css?family=Noto Sans' rel='stylesheet'>
<link href="
https://cdn.jsdelivr.net/npm/jetbrains-mono@1.0.6/css/jetbrains-mono.min.css
" rel="stylesheet">

<h1 style="text-align: center;">QSugar2🌱</h1>

<div style="text-align:center; letter-spacing: 0.05em;">
    响应式 Qt框架，简单且友好
</div>

<h2>
    简述⭐
</h2>

QSugar2 是继 [QSugar](https://github.com/niwik-dev/QSugar) 设计以来 的第二个独立版本。相较于上个版本，QSugar2 通过使用 Qt 元类型系统，摆脱了对 Python 动态特性的依赖，成为通用于 Qt 及其他语言绑定的框架。

目前，QSugar2 支持以下特性：
* 数据单向绑定
* 数据双向绑定
* 计算属性 及 绑定

框架的Python实现，见[QtForPython版本](https://github.com/niwik-dev/QSugar2/tree/main/python)。

<h2>
    教程📚
</h2>

QSugar2 沿用了`Ref`的概念。`Ref`是基本的属性包装类型，这个类型提供了基本的`setter`、`getter`方法以及`notify`信号，并且会在`setter`方法被调用时触发`notify`信号。

```c++
Ref<int> intVal(0);

qInfo()<<intVal.value(); // 输出 0

intVal.onChanged([](const int& val)){
    qInfo()<<val;
})
// 绑定属性的notify信号 和 槽函数

intVal = 1; // 或者 a.setValue(1);
// 由于 intVal的value被修改，所以会触发 onChanged 信号，输出 1
```

QSugar2 使用 `BIND` 宏来绑定属性。`BIND` 宏会根据属性的类型自动选择绑定类型。

```c++
Ref<QString> labelText("Hi,World");

QLabel*label = new QLabel();
BIND(label, text, labelText);
// BIND宏的参数分别是 控件对象、控件属性名、绑定后的包装对象

labelText = "Hello,World";
// 当 labelText的value被修改，label的text属性也会被修改

qInfo()<<label->text(); // 输出 Hello,World
```

对于支持交互的控件，QSugar2 提供了 `BIND2` 宏，来实现数据的双向绑定。

```c++
Ref<QString> content;

QLineEdit*lineEdit = new QLineEdit();
BIND2(lineEdit, text, content);
// BIND2宏 和 BIND宏 的参数一致，分别是 控件对象、控件属性名、绑定后的包装对象

content = "Hello,World";
// 当 content的value被修改，lineEdit的text属性也会被修改

/**
 * 当 用户在 lineEdit 控件内输入内容时，content的value也会被修改
 * 可以通过下面的代码来监听用户输入的内容
 */

content.onChanged([](const QString& val){
    qInfo()<<val;
})
// 当 content的value被修改，会触发 onChanged 信号，输出用户输入的内容
// 用户输入 "Hi,World"，输出 "Hi,World"
```

QSugar2 支持计算属性。通过 `Computed` 类来定义计算属性，可以实现复杂属性的逻辑。

`Computed`类 计算属性在定义时，需要显示地声明依赖的`Ref`类 属性。
```c++
Ref<int> a(1),b(2);
// 创建两个 Ref 类属性 a,b

Computed<QString> sum{a,b};
/** 另一种动态写法
Computed<QString> sum;
sum << a << b;
 **/

sum.setMethod([&](){
    return QString::number(a+b);
})
// 设置计算过程

sum.onChanged([](const QString& val){
    qInfo()<<val;
})

a = 2; // 当 a 的值被修改，会触发 sum 的 onChanged 信号，输出 2+2 => 4

qInfo() << sum(); // 也可以直接调用 仿函数 sum() 获取计算结果。
```

由于计算属性是 只读不可写的，因此只能使用 `BIND` 单向绑定到控件上。

```c++
Ref<int> a(1),b(2);
Computed<QString> sum{a,b};
sum.setMethod([&](){
    return QString::number(a+b);
});

QLabel*label = new QLabel();
BIND(label, text, sum);
// BIND 宏 和 上面的例子一致

a = 3; // 当 a 的值被修改，label 的 text 属性也会被修改

qInfo()<<label->text(); // 输出 3+2 => "5"
```

<h2>
    用法👨‍💻
</h2>
数据绑定，提供了数据(Model)和控件(View)分离的前提。

可以通过操作数据来直接控制控件的显示。

如下面的例子，一个简单的MVC例子。当然，Controller层是可选的。

```c++
... // 引入头文件

class Model{ // 数据模型
public:
    Ref<int> count(0);
    Computed<QString> text;
    
    Model(){
        text << count;
        text.setMethod([&](){
            return QString("Count: %1")
                .arg(count);
        })
    }
};

class ViewWidget : public QWidget{ // 视图层
    Q_OBJECT
public:
    ViewWidget(QWidget*parent = nullptr):QWidget(parent){
        QVBoxLayout*layout = new QVBoxLayout(this);
        QLabel*label = new QLabel(this);
        layout->addWidget(label);
    }
}

void bindModelAndView(Model&model,ViewWidget*view){
    BIND(view, text, model.text);
}

class Controller : public QObject{ // 控制层
    Q_OBJECT
private:
    Model *m_model;
public:
    Controller(QObject*parent = nullptr, Model* model):QObject(parent){
        m_model = model;
    }
    
    void inscrease(){
        m_model->count++;
    }
    void decrease(){
        m_model->count--;
    }
}

int main(int argc, char**argv){
    QApplication app(argc,argv);
    Model model;
    ViewWidget view;

    bindModelAndView(model,&view);
    
    Controller controller(&model);
    controller.inscrease();
    
    view.show();
    return app.exec();
}
```

<h2>
    例子📝
</h2>

见 `main.cpp`，实现了双滚动条计算求和展示的例子。

<h2>
    支持项目👍
</h2>

<p>
如果你觉得项目有价值，请给项目点个Star⭐，谢谢你啦！

如果你给下水道的鼠鼠买一杯热可可，

我会非常感谢你，并且在搬砖之余与你分享技术细节。
</p>

<h2>
    联系方式 📧
</h2>

<p>
gmail邮箱： niwik.dev@gmail.com
</p>
