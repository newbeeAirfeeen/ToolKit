
#ifndef OAHO_NOCOPYABLE_H
#define OAHO_NOCOPYABLE_H
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable(const noncopyable& that) = delete;
    noncopyable(noncopyable&& that) = delete;
    noncopyable& operator=(const noncopyable& that) = delete;
    noncopyable& operator=(noncopyable&& that) = delete;
};
#endif