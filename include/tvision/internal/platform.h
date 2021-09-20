#ifndef PLATFORM_H
#define PLATFORM_H

#define Uses_TPoint
#include <tvision/tv.h>
#include <internal/events.h>
#include <memory>
#include <stddef.h>

class DisplayStrategy {

public:

    virtual ~DisplayStrategy() {}

    // Invariant: these variables are is in sync with the actual screen state
    // after 'DisplayStrategy::reloadScreenInfo()' has been invoked.

    TPoint size {};
    int caretSize {};

    // This function can be overriden by inheritors in order to update
    // internal structs when the display properties change. But
    // 'DisplayStrategy::reloadScreenInfo()' must be invoked eventually.

    virtual void reloadScreenInfo() noexcept
    {
        size = getScreenSize();
        caretSize = getCaretSize();
    }

protected:

    // These functions are meant to be overriden by inheritors. Their only
    // responsibility should be to return updated values.

    virtual TPoint getScreenSize() noexcept { return size; }

    // 'getCaretSize()' must return a value in the range 0 to 100. Zero means
    // the caret is not visible. Note that PlatformStrategy::getCaretSize()
    // adjusts this to the range 1 to 100, because that's what the original
    // THardwareInfo::getCaretSize() did, and what TScreen expects.

    virtual int getCaretSize() noexcept { return caretSize; }

public:

    bool isCaretVisible() noexcept { return caretSize != 0; }
    virtual void clearScreen() noexcept {}
    virtual void setCaretPosition(int x, int y) noexcept {}
    virtual ushort getScreenMode() noexcept { return 0; }
    virtual void setCaretSize(int size) noexcept {}
    virtual void screenWrite(int x, int y, TScreenCell *buf, int len) noexcept {}
    virtual void flushScreen() noexcept {}

};

struct TEvent;

class InputStrategy : public EventSource
{
public:

    InputStrategy(SysHandle aHandle) noexcept :
        EventSource(aHandle)
    {
    }

    virtual ~InputStrategy() {}
    virtual bool getEvent(TEvent &ev) noexcept { return false; }
    virtual int getButtonCount() noexcept { return 0; }
    virtual void cursorOn() noexcept {}
    virtual void cursorOff() noexcept {}
};

class PlatformStrategy {

protected:

    std::unique_ptr<DisplayStrategy> display;
    std::unique_ptr<InputStrategy> input;

public:

    PlatformStrategy() noexcept {}
    PlatformStrategy( DisplayStrategy *d, InputStrategy *i ) noexcept :
        display(d),
        input(i)
    {
        if (display) display->reloadScreenInfo();
    }

    virtual ~PlatformStrategy() {}

    virtual bool getEvent(TEvent &ev) noexcept = 0;
    virtual void waitForEvents(int ms) noexcept = 0;
    virtual void stopEventWait() noexcept = 0;
    virtual int getButtonCount() noexcept { return input ? input->getButtonCount() : 0; }
    virtual void cursorOn() noexcept { if (input) input->cursorOn(); }
    virtual void cursorOff() noexcept { if (input) input->cursorOff(); }

    virtual int getCaretSize() noexcept { return min(max(display->caretSize, 1), 100); }
    virtual bool isCaretVisible() noexcept { return display->isCaretVisible(); }
    virtual void clearScreen() noexcept { display->clearScreen(); }
    virtual int getScreenRows() noexcept { return display->size.y; }
    virtual int getScreenCols() noexcept { return display->size.x; }
    virtual void setCaretPosition(int x, int y) noexcept { display->setCaretPosition(x, y); }
    virtual ushort getScreenMode() noexcept { return display->getScreenMode(); }
    virtual void setCaretSize(int size) noexcept { display->setCaretSize(size); }
    virtual void screenWrite(int x, int y, TScreenCell *buf, int len) noexcept { display->screenWrite(x, y, buf, len); }
    virtual void flushScreen() noexcept { display->flushScreen(); }
    virtual void reloadScreenInfo() noexcept { display->reloadScreenInfo(); }
    virtual int charWidth(TStringView mbc, char32_t wc) noexcept = 0;

    static PlatformStrategy *instance;

};

class NullPlatform : public PlatformStrategy {

    NullPlatform() = default;
    ~NullPlatform() = default;

public:

    bool getEvent (TEvent &) noexcept override { return false; }
    void waitForEvents(int ms) noexcept override {}
    void stopEventWait() noexcept override {}
    int getButtonCount() noexcept override { return 0; }
    void cursorOn() noexcept override {}
    void cursorOff() noexcept override {}

    int getCaretSize() noexcept override { return 0; }
    bool isCaretVisible() noexcept override { return false; }
    void clearScreen() noexcept override {}
    int getScreenRows() noexcept override { return 0; }
    int getScreenCols() noexcept override { return 0; }
    void setCaretPosition(int x, int y) noexcept override {}
    ushort getScreenMode() noexcept override { return 0; }
    void setCaretSize(int size) noexcept override {}
    void screenWrite(int x, int y, TScreenCell *buf, int len) noexcept override {}
    void flushScreen() noexcept override {}
    void reloadScreenInfo() noexcept override {}
    int charWidth(TStringView mbc, char32_t wc) noexcept override { return 1; }

    static NullPlatform instance;

};

#ifdef _TV_UNIX

#include <internal/sigwinch.h>

class UnixPlatformStrategy : public PlatformStrategy
{
    std::unique_ptr<SigwinchHandler> sigwinch;

protected:

    EventWaiter waiter;

public:

    UnixPlatformStrategy(DisplayStrategy &d, InputStrategy &i) noexcept :
        PlatformStrategy(&d, &i),
        sigwinch(SigwinchHandler::create())
    {
        waiter.addSource(i);
        if (sigwinch)
            waiter.addSource(sigwinch->getEventSource());
    }

    bool getEvent(TEvent &ev) noexcept override
    {
        return waiter.getEvent(ev);
    }

    void waitForEvents(int ms) noexcept override
    {
        waiter.waitForEvents(ms);
    }

    void stopEventWait() noexcept override
    {
        waiter.stopEventWait();
    }

    int charWidth(TStringView mbc, char32_t wc) noexcept override; // ttext.cpp
};

#endif // _TV_UNIX

#endif // PLATFORM_H
