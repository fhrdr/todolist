#ifndef FONTAWESOME_H
#define FONTAWESOME_H

#include <QIcon>
#include <QFont>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QApplication>
#include <QDir>
#include <QFontDatabase>

class FontAwesome
{
public:
    static FontAwesome& instance()
    {
        static FontAwesome instance;
        return instance;
    }
    
    bool initialize()
    {
        if (m_initialized) return true;
        
        QStringList fontPaths = {
            QApplication::applicationDirPath() + "/fonts/Font Awesome 7 Free-Solid-900.otf",
            QApplication::applicationDirPath() + "/fonts/fa-solid-900.ttf",
            QApplication::applicationDirPath() + "/Font Awesome 7 Free-Solid-900.otf",
            QApplication::applicationDirPath() + "/fa-solid-900.ttf",
            "fonts/Font Awesome 7 Free-Solid-900.otf",
            "fonts/fa-solid-900.ttf"
        };
        
        for (const QString &fontPath : fontPaths) {
            if (QFile::exists(fontPath)) {
                int fontId = QFontDatabase::addApplicationFont(fontPath);
                if (fontId >= 0) {
                    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
                    if (!families.isEmpty()) {
                        m_fontFamily = families.at(0);
                        m_initialized = true;
                        return true;
                    }
                }
            }
        }
        
        m_initialized = false;
        return false;
    }
    
    QFont font(int pixelSize = 14)
    {
        if (m_initialized && !m_fontFamily.isEmpty()) {
            QFont f(m_fontFamily);
            f.setPixelSize(pixelSize);
            return f;
        }
        return QFont();
    }
    
    QString icon(IconCode code)
    {
        return QChar(static_cast<int>(code));
    }
    
    enum IconCode {
        Plus = 0xf067,
        Minus = 0xf068,
        Times = 0xf00d,
        Check = 0xf00c,
        Edit = 0xf044,
        Trash = 0xf1f8,
        Folder = 0xf07b,
        FolderOpen = 0xf07c,
        Calendar = 0xf133,
        Tag = 0xf02b,
        Tags = 0xf02c,
        Pin = 0xf08d,
        Star = 0xf005,
        Home = 0xf015,
        Cog = 0xf013,
        Sync = 0xf021,
        Search = 0xf002,
        Bell = 0xf0f3,
        List = 0xf03a,
        Bars = 0xf0c9,
        ChevronLeft = 0xf053,
        ChevronRight = 0xf054,
        ChevronUp = 0xf077,
        ChevronDown = 0xf078,
        AngleLeft = 0xf104,
        AngleRight = 0xf105,
        AngleUp = 0xf106,
        AngleDown = 0xf107,
        ArrowLeft = 0xf060,
        ArrowRight = 0xf061,
        ArrowUp = 0xf062,
        ArrowDown = 0xf063,
        Save = 0xf0c7,
        Download = 0xf019,
        Upload = 0xf093,
        Export = 0xf56e,
        Import = 0xf56f,
        Eye = 0xf06e,
        EyeSlash = 0xf070,
        Lock = 0xf023,
        Unlock = 0xf09c,
        User = 0xf007,
        Users = 0xf0c0,
        Heart = 0xf004,
        Flag = 0xf024,
        Book = 0xf02d,
        Bookmark = 0xf02e,
        Clock = 0xf017,
        History = 0xf1da,
        Filter = 0xf0b0,
        Sort = 0xf0dc,
        SortUp = 0xf0de,
        SortDown = 0xf0dd,
        Refresh = 0xf021,
        Redo = 0xf01e,
        Undo = 0xf0e2,
        Copy = 0xf0c5,
        Paste = 0xf0ea,
        Cut = 0xf0c4,
        WindowMaximize = 0xf2d0,
        WindowMinimize = 0xf2d1,
        WindowRestore = 0xf2d2,
        WindowClose = 0xf410,
        Desktop = 0xf108,
        Laptop = 0xf109,
        Mobile = 0xf10b,
        Tablet = 0xf10a,
        Info = 0xf129,
        InfoCircle = 0xf05a,
        Question = 0xf128,
        QuestionCircle = 0xf059,
        Exclamation = 0xf12a,
        ExclamationCircle = 0xf06a,
        ExclamationTriangle = 0xf071,
        CheckCircle = 0xf058,
        TimesCircle = 0xf057,
        PlusCircle = 0xf055,
        MinusCircle = 0xf056
    };

private:
    FontAwesome() : m_initialized(false) {}
    FontAwesome(const FontAwesome&) = delete;
    FontAwesome& operator=(const FontAwesome&) = delete;
    
    bool m_initialized;
    QString m_fontFamily;
};

#endif // FONTAWESOME_H
