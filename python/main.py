from PySide6.QtCore import Qt
from PySide6.QtWidgets import QWidget, QLabel, QSlider, QVBoxLayout, QApplication

from binds import BIND2, BIND, Ref, Computed


class Model:

    def __init__(self):
        super().__init__()
        self.valueA = Ref(int)

        self.valueB = Ref(int)
        self.message = Computed(str)
        self.display = Ref(str)

        self.init()

    def init(self):
        self.message << self.valueA << self.valueB
        self.message.setMethod(
            lambda: f"{~self.valueA}+ {~self.valueB} = {~self.valueA + ~self.valueB}"
        )


class TestWidget(QWidget):
    def __init__(self, *args, **kwargs):
        super().__init__(*args,**kwargs)
        layout = QVBoxLayout(self)

        self._label = QLabel(self)
        layout.addWidget(self._label)

        self._sliderA = QSlider(self)
        self._sliderA.setOrientation(Qt.Orientation.Horizontal)
        layout.addWidget(self._sliderA)

        self._sliderB = QSlider(self)
        self._sliderB.setOrientation(Qt.Orientation.Horizontal)
        layout.addWidget(self._sliderB)

    def bindOn(self, model: Model):
        BIND2(self._sliderA, "value", model.valueA)
        BIND2(self._sliderB, "value", model.valueB)

        BIND(self._label, "text", model.message)
        BIND(self._label, "text", model.display)


if __name__ == '__main__':
    app = QApplication([])
    model = Model()

    model.display <<= "No Data"

    widget = TestWidget()
    widget.bindOn(model)
    widget.show()

    app.exec()
