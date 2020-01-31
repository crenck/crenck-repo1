#ifndef __AutoQPointer_H__
#define __AutoQPointer_H__

/* AutoQPointer is design to provide ease way the commonly used pattern:

SomeQDialog dlg;
if (dlg.exec() != QDialog::Accepted)
{
    return false;
}
dlg.doSomething();

to be replaced with safes pattern:

AutoQpointer<SomeQDialog> dlg(new SomeQDialog());
if (dlg->exec() != QDialog::Accepted || !dlg)
{
    return false;
}
dlg->doSomething();
*/

#include <QPointer>

template <class T>
class AutoQPointer : public QPointer<T>
{
public:
    AutoQPointer() {}
    AutoQPointer(T * p) : QPointer<T>(p) {}
    ~AutoQPointer()
    {
        delete QPointer<T>::data();
    }
};

#endif // __AutoQPointer_H__
