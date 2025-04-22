#pragma once
#include <stdint.h>

#include <QImage>


//Простенький враппер для изображения, в котором есть только alpha канал
class AImage
{
public:
    AImage(const QImage& img);
    QImage qalpha() const;
    QImage qgray() const;

    AImage();
    AImage(int width, int height, uint8_t fill=0);
    virtual ~AImage();

    AImage(AImage&& src);
    AImage(const AImage&) = delete;
    AImage& operator= (const AImage&) = delete;
    void operator=(AImage&&);

    int width() const { return _width; }
    int height() const { return _height; }

    bool in(int x, int y) const { return x >= 0 && x < _width && y >= 0 && y < _height; }
    //Зв переделами изображения падает по assert
    uint8_t get(int x, int y) const;
    //За переделами изображения возвращает 0
    uint8_t getb(int x, int y) const;
    void set(int x, int y, uint8_t value);

protected:
    int _width = 0;
    int _height = 0;
    uint8_t* _data = nullptr;
};

class AImage32
{
public:
    QImage qpal() const;

    AImage32();
    AImage32(int width, int height, uint8_t fill=0);
    virtual ~AImage32();

    AImage32(AImage32&& src);
    AImage32(const AImage32&) = delete;
    AImage32& operator= (const AImage32&) = delete;
    void operator=(AImage32&&);

    int width() const { return _width; }
    int height() const { return _height; }

    bool in(int x, int y) const { return x >= 0 && x < _width && y >= 0 && y < _height; }
    //Зв переделами изображения падает по assert
    uint32_t get(int x, int y) const;
    //За переделами изображения возвращает 0
    uint32_t getb(int x, int y) const;
    void set(int x, int y, uint32_t value);

protected:
    int _width = 0;
    int _height = 0;
    uint32_t* _data = nullptr;
};
