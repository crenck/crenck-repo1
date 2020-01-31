#ifndef DIMENSION_THUMB__H
#define DIMENSION_THUMB__H

#include <QJsonObject>
#include <QString>

namespace Thumb
{

class Dimension
{
public:
    Dimension();

    void setDimension(int width, int height);

    int width() const;
    int height() const;

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;

private:
    int mWidth;
    int mHeight;
};

}

#endif // DIMENSION_THUMB__H
