
#ifndef OAHO_NOCOPYABLE_H
#define OAHO_NOCOPYABLE_H
//½ûÖ¹¿½±´»ùÀà
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    //½ûÖ¹¿½±´
    noncopyable(const noncopyable& that) = delete;
    noncopyable(noncopyable&& that) = delete;
    noncopyable& operator=(const noncopyable& that) = delete;
    noncopyable& operator=(noncopyable&& that) = delete;
};
#endif