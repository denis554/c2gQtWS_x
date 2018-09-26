#include "unsafearea.hpp"

#include <QDebug>

UnsafeArea::UnsafeArea(QObject *parent) :
        QObject(parent), mUnsafeTopMargin(0), mUnsafeBottomMargin(0), mUnsafeLeftMargin(0), mUnsafeRightMargin(0)
{ 

}

// https://bugreports.qt.io/browse/QTBUG-64574
// I O S sizes to detect the device type
// https://stackoverflow.com/questions/46192280/detect-if-the-device-is-iphone-x
// 1136 iPhone 5, 5S, 5C
// 1334 iPhone 6/6S/7/8
// 1920,2208 iPhone 6+/6S+/7+/8+
// 2436 iPhone X, Xs
// 2688 iPhone Xs Max
// 1792 iPhone Xr
void UnsafeArea::configureDevice(int height, int width, int devicePixelRatio)
{
    qDebug() << "configureDevice - height: " << height << " width: " << width << " devicePixelRatio: " << devicePixelRatio;
    int portraitHeightPixel = 0;
    if(height > width) {
        portraitHeightPixel = height*devicePixelRatio;
    } else {
        portraitHeightPixel = width*devicePixelRatio;
    }
    switch (portraitHeightPixel) {
    case 1136:
        mMyDevice = MyDevice::IPHONE_5_5S_5C;
        qDebug() << "Device detected: " << "IPHONE_5_5S_5C";
        break;
    case 1334:
        mMyDevice = MyDevice::IPHONE_6_6S_7_8;
        qDebug() << "Device detected: " << "IPHONE_6_6S_7_8";
        break;
    case 1920:
    case 2208:
        mMyDevice = MyDevice::IPHONE_6PLUS_6SPLUS_7PLUS_8PLUS;
        qDebug() << "Device detected: " << "IPHONE_6PLUS_6SPLUS_7PLUS_8PLUS";
        break;
    case 2436:
        mMyDevice = MyDevice::IPHONE_X_XS;
        qDebug() << "Device detected: " << "IPHONE_X_XS";
        break;
    case 2688:
        mMyDevice = MyDevice::IPHONE_XSMAX;
        qDebug() << "Device detected: " << "IPHONE_XSMAX";
        break;
    case 1792:
        mMyDevice = MyDevice::IPHONE_XR;
        qDebug() << "Device detected: " << "IPHONE_XR";
        break;
    default:
        mMyDevice = MyDevice::OTHER;
        qDebug() << "Device detected: " << "OTHER";
    }
}

void UnsafeArea::orientationChanged(int orientation)
{
    qDebug() << "orientationChanged: " << orientation;
    if(orientation == 1) {
        qDebug() << "PORTRAIT";
    } else if(orientation == 2) {
        qDebug() << "LANDSCAPE LEFT (HomeButton right)";
    } else if(orientation == 8) {
        qDebug() << "LANDSCAPE RIGHT (HomeButton left)";
    } else {
        qWarning() << "unsupported Orientation: " << orientation;
    }
}

int UnsafeArea::unsafeTopMargin() const
{
    return mUnsafeTopMargin;
}

int UnsafeArea::unsafeBottomMargin() const
{
    return mUnsafeBottomMargin;
}

int UnsafeArea::unsafeLeftMargin() const
{
    return mUnsafeLeftMargin;
}

int UnsafeArea::unsafeRightMargin() const
{
    return mUnsafeRightMargin;
}

void UnsafeArea::setUnsafeTopMargin(int unsafeTopMargin)
{
    if (mUnsafeTopMargin == unsafeTopMargin)
        return;
    mUnsafeTopMargin = unsafeTopMargin;
    emit unsafeTopMarginChanged(mUnsafeTopMargin);
}
void UnsafeArea::setUnsafeBottomMargin(int unsafeBottomMargin)
{
    if (mUnsafeBottomMargin == unsafeBottomMargin)
        return;
    mUnsafeBottomMargin = unsafeBottomMargin;
    emit unsafeBottomMarginChanged(mUnsafeBottomMargin);
}
void UnsafeArea::setUnsafeLeftMargin(int unsafeLeftMargin)
{
    if (mUnsafeLeftMargin == unsafeLeftMargin)
        return;
    mUnsafeLeftMargin = unsafeLeftMargin;
    emit unsafeLeftMarginChanged(mUnsafeLeftMargin);
}
void UnsafeArea::setUnsafeRightMargin(int unsafeRightMargin)
{
    if (mUnsafeRightMargin == unsafeRightMargin)
        return;
    mUnsafeRightMargin = unsafeRightMargin;
    emit unsafeRightMarginChanged(mUnsafeRightMargin);
}

UnsafeArea::~UnsafeArea()
{
    // place cleanUp code here
}
