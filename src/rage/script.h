#ifndef SCRIPT_H
#define SCRIPT_H

#include <memory>
#include <QFile>
#include <QString>

#include "opcodefactory.h"

struct ResourceHeader
{
    unsigned int magic;
    int version;
    int flags1;
    int flags2;

    int vSize;
    int pSize;
    int _f14_30;
    bool extended;

    int getSizeV() { return extended ? (vSize << 12) : ((int)(flags1 & 0x7FF) << ((int)((flags1 >> 11) & 15) + 8)); }
    int getSizeP() { return extended ? (pSize << 12) : ((int)((flags1 >> 15) & 0x7FF) << ((int)((flags1 >> 26) & 15) + 8)); }
};

struct ScriptHeader
{
    int magic;
    int pageMapPtr;
    int codeSize;
    int paramCount;

    int staticsSize;
    int staticsPtr;

    int globalsVers;
    int nativesSize;
    int nativesPtr;

    int codePagesSize;
    int codePagesPtr;
    std::vector<int> codePages;

    int headerPos;
};

class Script
{
public:
    Script(QString path);

    QByteArray getData() { return m_data; };

    ResourceHeader getResourceHeader() { return m_header;       }
    ScriptHeader   getScriptHeader()   { return m_scriptHeader; }

    std::list<std::shared_ptr<IOpcode>> getOpcodes()     { return m_opcodes;   }
    std::vector<std::shared_ptr<IOpcode>> getFunctions() { return m_functions; }
    std::vector<std::shared_ptr<IOpcode>> getStrings()   { return m_strings;   }
    std::map<unsigned int, QString> getJumps()           { return m_jumps;     }
    QVector<unsigned int> getNatives()                   { return m_natives;   }

private:
    // Extract script from RSC container
    bool readRSCHeader();
    void extractData();

    // Read script data
    int  findScriptHeader();
    void readScriptHeader(int headerPos);
    void readNatives();
    void readStatics();

    void readPages();
    void readPage(int address, int page);

    // Resource data
    ResourceHeader m_header;

    // Script data
    ScriptHeader m_scriptHeader;
    QVector<unsigned int> m_natives;

    std::list<std::shared_ptr<IOpcode>>   m_opcodes;
    std::vector<std::shared_ptr<IOpcode>> m_functions;
    std::vector<std::shared_ptr<IOpcode>> m_strings;
    std::map<unsigned int, QString>       m_jumps;

    // General data
    QByteArray m_data;
    QFile m_script;
};

#endif // SCRIPT_H
