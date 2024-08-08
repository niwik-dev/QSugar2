from typing import TypeVar, Generic, Callable, Any, Optional, cast, Union, get_args

from PySide6.QtCore import QObject, Signal, Property, QEvent, QMetaObject, Q_ARG

T = TypeVar('T')


class RefListener(QObject):
    valueChanged = Signal(object)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._propertyName = str()

    def setPropertyName(self, propertyName: str):
        self._propertyName = propertyName

    def eventFilter(self, watched: QObject, event: QEvent) -> bool:
        if event.type() & QEvent.Type.InputMethod:
            self.valueChanged.emit(watched.property(self._propertyName))
        return super().eventFilter(watched, event)


class RefImpl(QObject, Generic[T]):
    def __init__(self, type_: type, value: Optional[T] = None):
        super().__init__()
        self.__pack__ = False

        self._value = value or type_()
        self._listener: Optional[RefListener] = None

    valueChanged = Signal(object)

    def __ilshift__(self, value: T):
        self.setValue(value)

    def __invert__(self) -> T:
        return self._value

    def getValue(self) -> T:
        return self._value

    def setValue(self, value: T) -> None:
        if self._value == value:
            return
        self._value = value
        self.valueChanged.emit(value)

    value = Property(object, fget=getValue, fset=setValue)

    def onChanged(self, callback: Callable[[T], None]) -> None:
        self.valueChanged.connect(callback)

    def listenOn(self, target: QObject, propertyName: str):
        if self._listener is None:
            self._listener = RefListener(self)
            assert self._listener is not None
        self._listener.setPropertyName(propertyName)
        self._listener.valueChanged.connect(lambda value: self.setValue(value))
        target.installEventFilter(self._listener)


ReturnType = TypeVar('ReturnType')


def Ref(type_: type, value: object = None) -> RefImpl:
    if value is None:
        value = type_()
    return RefImpl[type_](type_, value)


class ComputedImpl(QObject, Generic[ReturnType]):
    updated = Signal(object)

    def __init__(self, *args: RefImpl[Any]):
        super().__init__()

        self._method: Optional[Callable[[], ReturnType]] = None
        self.addRef(*args)

    def addRef(self, *refObjs: RefImpl[Any]):
        for refObj in refObjs:
            refObj.onChanged(self.updated.emit)

    def __lshift__(self, refObj: RefImpl[Any]):
        self.addRef(refObj)
        return self

    def onChanged(self, callback: Callable[[Any], None]) -> None:
        self.updated.connect(callback)

    def setMethod(self, method: Callable[[], ReturnType]):
        self._method = method

    def __call__(self) -> ReturnType:
        if self._method is None:
            raise RuntimeError("ComputedImpl method is not set")
        return self._method()


def Computed(type_: type) -> ComputedImpl:
    return ComputedImpl[type_]()


def bindRef(target: QObject, propertyName: str, refProp: RefImpl[T]) -> None:
    metaObject = target.metaObject()
    for _ in range(1):
        propertyIndex = metaObject.indexOfProperty(propertyName)
        if propertyIndex != -1:
            target.setProperty(propertyName, refProp.getValue())

            def callBack(value: T):
                target.setProperty(propertyName, refProp.getValue())

            refProp.onChanged(callBack)
            break

        methodIndex = metaObject.indexOfMethod(QMetaObject.normalizedSignature(propertyName.encode()))
        if methodIndex != -1:
            method = metaObject.method(methodIndex)
            method.invoke(target, Q_ARG(T, refProp.getValue()))

            def callBack(value: T):
                method.invoke(target, Q_ARG(T, refProp.getValue()))

            refProp.onChanged(callBack)
            break


def bindComputed(target: T, propertyName: str, computedProp: ComputedImpl[T]) -> None:
    metaObject = target.metaObject()
    for _ in range(1):
        propertyIndex = metaObject.indexOfProperty(propertyName)
        if propertyIndex != -1:
            target.setProperty(propertyName, computedProp())
            computedProp.onChanged(lambda value: target.setProperty(propertyName, computedProp()))
            break

        methodIndex = metaObject.indexOfMethod(QMetaObject.normalizedSignature(propertyName.encode()))
        if methodIndex != -1:
            method = metaObject.method(methodIndex)
            method.invoke(target, Q_ARG(T, computedProp()))
            computedProp.onChanged(lambda value: method.invoke(target, Q_ARG(T, computedProp())))
            break


def BIND(target: QObject, propertyName: str, prop: Union[RefImpl, ComputedImpl]):
    if isinstance(prop, RefImpl):
        bindRef(target, propertyName, prop)
    if isinstance(prop, ComputedImpl):
        bindComputed(target, propertyName, prop)


def BIND2(target: QObject, propertyName: str, refProp: RefImpl[Any]):
    bindRef(target, propertyName, refProp)
    refProp.listenOn(target, propertyName)
