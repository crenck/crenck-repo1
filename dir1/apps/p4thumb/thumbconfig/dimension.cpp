#include "dimension.h"

namespace Thumb
{

const char* WIDTH ="width";
const char* HEIGHT = "height";

Dimension::Dimension() :
    mWidth(160),
    mHeight(160)
{
}

void Dimension::setDimension(const int width, const int height)
{
    mWidth = width;
    mHeight = height;
}

int Dimension::width() const
{
    return mWidth;
}

int Dimension::height() const
{
    return mHeight;
}

void Dimension::read(const QJsonObject& json)
{
    if (json.contains(WIDTH))
    {
        mWidth = json[WIDTH].toInt();
    }
    if (json.contains(HEIGHT))
    {
        mHeight = json[HEIGHT].toInt();
    }
}

void Dimension::write(QJsonObject& json) const
{
    json[WIDTH] = mWidth;
    json[HEIGHT] = mHeight;
}

}

