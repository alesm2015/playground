#pragma once

#include <cli/cli.h>
#include <cli/scheduler.h>
#include <cli/detail/inputdevice.h>
#include <cli/detail/commandprocessor.h>
#include <cli/detail/screen.h>
#include <cli/colorprofile.h>


namespace cli
{

// *******************************************************************************
enum BeforeError { beforeError };
enum AfterError { afterError };

inline std::ostream& operator<<(std::ostream& os, BeforeError)
{
    if ( Color() ) { os << detail::rang::control::forceColor << detail::rang::fg::red << detail::rang::style::bold; }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, AfterError)
{
    os << detail::rang::style::reset;
    return os;
}


class CliCustomInputDevice: public detail::InputDevice
{
private:
    enum class Step { _1, _2, _3, _4, wait_0 };

public:
    explicit CliCustomInputDevice(Scheduler& _scheduler) :\
        detail::InputDevice(_scheduler)
    {
        m_step = Step::_1;
    }

    virtual void Read(const std::vector<uint8_t> &data) noexcept
    {
        for (auto &byte : data) {
            input(static_cast<char>(byte));
        }
    }

private:
    void input(char c) // NB: C++ does not specify wether char is signed or unsigned
    {
        switch(m_step)
        {
        case Step::_1:
            switch( c )
            {
            case static_cast<char>(EOF):
            case 4:  // EOT
                Notify(std::make_pair(detail::KeyType::eof,' ')); break;
            case 8: // Backspace
            case 127:  // Backspace or Delete
                Notify(std::make_pair(detail::KeyType::backspace, ' ')); break;
            //case 10: Notify(std::make_pair(KeyType::ret,' ')); break;
            case 12: // ctrl+L
                Notify(std::make_pair(detail::KeyType::clear, ' ')); break;
            case 27: m_step = Step::_2; break;  // symbol
            case 13: m_step = Step::wait_0; break;  // wait for 0 (ENTER key)
            default: // ascii
            {
                const char ch = static_cast<char>(c);
                Notify(std::make_pair(detail::KeyType::ascii,ch));
            }
            }
            break;

        case Step::_2: // got 27 last time
            if ( c == 91 )
            {
                m_step = Step::_3;
                break;  // arrow keys
            }
            else
            {
                m_step = Step::_1;
                Notify(std::make_pair(detail::KeyType::ignored,' '));
                break; // unknown
            }
            break;

        case Step::_3: // got 27 and 91
            switch( c )
            {
            case 65: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::up,' ')); break;
            case 66: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::down,' ')); break;
            case 68: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::left,' ')); break;
            case 67: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::right,' ')); break;
            case 70: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::end,' ')); break;
            case 72: m_step = Step::_1; Notify(std::make_pair(detail::KeyType::home,' ')); break;
            default: m_step = Step::_4; break;  // not arrow keys
            }
            break;

        case Step::_4:
            if ( c == 126 ) Notify(std::make_pair(detail::KeyType::canc,' '));
            else Notify(std::make_pair(detail::KeyType::ignored,' '));

            m_step = Step::_1;

            break;

        case Step::wait_0:
            if ( c == 0 /* linux */ || c == 10 /* win */ ) Notify(std::make_pair(detail::KeyType::ret,' '));
            else Notify(std::make_pair(detail::KeyType::ignored,' '));

            m_step = Step::_1;

            break;
        }
    }

    private:
        Step m_step = Step::_1;
};


class CliCustomSession: public std::streambuf
{
    std::ostream outStream;

public:
    explicit CliCustomSession(void): outStream( this )
    {
    }

    virtual void Send (const std::string& _data) const
    {
        (void)(_data);
    }

protected:
    virtual std::ostream& OutStream() { return outStream; }

    virtual std::string Encode(const std::string& _data) const
    {
        std::string result;
        for (char c: _data)
        {
            if (c == '\n') result += '\r';
            result += c;
        }
        return result;
    }

private:
    // std::streambuf
    std::streamsize xsputn( const char* s, std::streamsize n ) override
    {
        Send(Encode(std::string(s, s+n)));
        return n;
    }

    int overflow( int c ) override
    {
        Send(Encode(std::string(1, static_cast< char >(c))));
        return c;
    }

};

class CliCustomLoopScheduler : public Scheduler
{
public:
    CliCustomLoopScheduler() = default;
    ~CliCustomLoopScheduler() override
    {
    }

    // non copyable
    CliCustomLoopScheduler(const CliCustomLoopScheduler&) = delete;
    CliCustomLoopScheduler& operator=(const CliCustomLoopScheduler&) = delete;

    void Post(const std::function<void()>& f) override
    {
        f();
    }
};


class CliCustomTerminalSession : public CliCustomInputDevice, public CliCustomSession, public CliSession
{
public:
    CliCustomTerminalSession
    (
        Cli &_cli,
        Scheduler &_scheduler,
        const std::function<void(std::ostream&)>& _enterAction,
        const std::function<void(std::ostream&)>& _exitAction,
        const std::function<void(const std::string&)>& _sendAction,
        std::size_t historySize = 100
    ):\
        CliCustomInputDevice(_scheduler),
        CliCustomSession(),
        CliSession(_cli, CliCustomSession::OutStream(), historySize),
        m_sendAction{}
    {
        m_poll_ptr = std::make_unique<detail::CommandProcessor<detail::TelnetScreen>>(*this, *this);
        assert(m_poll_ptr != nullptr);

        EnterAction([enter = _enterAction](std::ostream& _out){ enter(_out); } );
        ExitAction([exit = _exitAction](std::ostream& _out){ exit(_out); } );
        SendAction([send = _sendAction](const std::string& msg) {send(msg);});

        SetColor();
    }

    void OnConnect(void)
    {
        Enter();
        Prompt();
    }

    void Read(const std::vector<uint8_t> &data) noexcept override
    {
        CliCustomInputDevice::Read(data);
    }

    void Send (const std::string& data) const override
    {
        if (m_sendAction)
            (m_sendAction)(data);
    }

private:
    void SendAction(const std::function<void(const std::string&)>& send_action)
    {
        m_sendAction = send_action;
    }


private:
    std::unique_ptr<detail::CommandProcessor<detail::TelnetScreen>> m_poll_ptr;
    std::function<void(const std::string&)> m_sendAction = []( const std::string& ) noexcept {};
};


// *******************************************************************************

};


