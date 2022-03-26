#include "disassembler.h"
#include "ui_disassembler.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "../util/util.h"
#include "../QHexView/qhexview.h"
#include "../QHexView/document/buffer/qmemoryrefbuffer.h"

Disassembler::Disassembler(QString file, QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::Disassembler)
    , m_script(file)
    , m_file(file)
{
    m_ui->setupUi(this);

    m_ui->funcTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_nativeMap = Util::getNatives();

    m_disasm = new OpcodeTable(0, 4, m_ui->tabWidget);
    m_ui->tabWidget->addTab(m_disasm, "Disassembly");

    fillDisassembly();
    createStringsTab();
    createScriptDataTab();

    connect(m_ui->actionExportDisassembly, SIGNAL(triggered()), this, SLOT(exportDisassembly()));
    connect(m_ui->actionExportRawData,     SIGNAL(triggered()), this, SLOT(exportRawData()));

    connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(exit()));
    connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(open()));

    setWindowTitle("RDRasm - " + file);

    QApplication::setOverrideCursor(Qt::ArrowCursor);
}

Disassembler::~Disassembler()
{
    delete m_ui;
}

void Disassembler::exportDisassembly()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export disassembly", m_file.split("\\").last().split(".xsc").first(), "Text (*.txt)");

    if (filePath.isEmpty())
    {
        return;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Error: unable to write to file. Make sure the file isn't open elsewhere.");
        return;
    }

    QString str;

    for (int row = 0; row < m_disasm->rowCount(); row++)
    {
        for (int col = 0; col < m_disasm->columnCount(); col++)
        {
            QTableWidgetItem *item = m_disasm->item(row, col);

            if (item != nullptr)
            {
                str += m_disasm->item(row, col)->text().leftJustified(15);
            }
        }

        str += "\n";
    }

    QTextStream stream(&file);

    stream << str;

    file.close();

    QMessageBox::information(this, "Exported", "Successfully exported to " + filePath);
}

void Disassembler::exportRawData()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export raw data", m_file.split("\\").last().split(".xsc").first(), "Binary data (*.bin)");

    if (filePath.isEmpty())
    {
        return;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Error: unable to write to file. Make sure the file isn't open elsewhere.");
        return;
    }

    file.write(m_script.getData());

    file.close();

    QMessageBox::information(this, "Exported", "Successfully exported to " + filePath);
}

void Disassembler::exit()
{
    QApplication::exit();
}

void Disassembler::open()
{
    QString file = QFileDialog::getOpenFileName(this, "Select a file", "", "Script (*.xsc)");

    if (!file.isEmpty())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        Disassembler *dsm = new Disassembler(file);
        dsm->show();

        close();
    }
}

void Disassembler::createScriptDataTab()
{
    QTextEdit *scriptData = new QTextEdit(this);

    scriptData->setFont(QFont("Roboto Mono", 10));

    m_ui->tabWidget->addTab(scriptData, "Script Data");

    scriptData->append(getResourceHeaderData());
    scriptData->append(getScriptHeaderData());
}

QString Disassembler::getResourceHeaderData()
{
    QString resourceData;

    resourceData.append("Resource data:\n");
    resourceData.append(QString("    magic: %1\n").arg(m_script.getResourceHeader().magic));
    resourceData.append(QString("    version: %1\n").arg(m_script.getResourceHeader().version));
    resourceData.append(QString("    flags1: %1\n").arg(m_script.getResourceHeader().flags1));
    resourceData.append(QString("    flags2: %1\n").arg(m_script.getResourceHeader().flags2));
    resourceData.append(QString("    vsize: %1\n").arg(m_script.getResourceHeader().vSize));
    resourceData.append(QString("    psize: %1\n").arg(m_script.getResourceHeader().pSize));
    resourceData.append(QString("    _f14_30: %1\n").arg(m_script.getResourceHeader()._f14_30));
    resourceData.append(QString("    extended: %1\n").arg(m_script.getResourceHeader().extended));
    resourceData.append(QString("    virtual size: %1\n").arg(m_script.getResourceHeader().getSizeV()));
    resourceData.append(QString("    physical size: %1\n\n").arg(m_script.getResourceHeader().getSizeP()));

    return resourceData;
}

QString Disassembler::getScriptHeaderData()
{
    QString scriptData;

    scriptData.append("Script data:\n");
    scriptData.append(QString("    headerPos: %1\n").arg(m_script.getScriptHeader().headerPos));
    scriptData.append(QString("    magic: %1\n").arg(m_script.getScriptHeader().magic));
    scriptData.append(QString("    pageMapPtr: %1\n").arg(m_script.getScriptHeader().pageMapPtr));
    scriptData.append(QString("    codeSize: %1\n").arg(m_script.getScriptHeader().codeSize));
    scriptData.append(QString("    paramCount: %1\n").arg(m_script.getScriptHeader().paramCount));
    scriptData.append(QString("    staticsSize: %1\n").arg(m_script.getScriptHeader().staticsSize));
    scriptData.append(QString("    staticsPtr: %1\n").arg(m_script.getScriptHeader().staticsPtr));
    scriptData.append(QString("    globalsVers: %1\n").arg(m_script.getScriptHeader().globalsVers));
    scriptData.append(QString("    nativesSize: %1\n").arg(m_script.getScriptHeader().nativesSize));
    scriptData.append(QString("    nativesPtr: %1\n").arg(m_script.getScriptHeader().nativesPtr));
    scriptData.append(QString("    codePagesSize: %1\n").arg(m_script.getScriptHeader().codePagesSize));
    scriptData.append(QString("    codePagesPtr: %1\n").arg(m_script.getScriptHeader().codePagesPtr));

    return scriptData;
}

QTableWidget *Disassembler::createStringsTab()
{
    QTableWidget *stringTable = new QTableWidget(m_script.getStrings().size(), 2, this);

    stringTable->setHorizontalHeaderLabels({"Location", "String"});
    stringTable->verticalHeader()->setVisible(false);
    stringTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);

    for (unsigned int i = 0; i < m_script.getStrings().size(); i++)
    {
        auto op = m_script.getStrings()[i];

        stringTable->setItem(i, 0, new QTableWidgetItem(op->getFormattedLocation()));
        stringTable->setItem(i, 1, new QTableWidgetItem(op->getArgsString()));
    }

    m_ui->tabWidget->addTab(stringTable, "Strings");

    return stringTable;
}

void Disassembler::fillDisassembly()
{
    int invalidCalls = 0;

    for (auto op : m_script.getOpcodes())
    {
        if (op->getOp() == EOpcodes::OP_NOP)
            continue;

        QTableWidgetItem *address = new QTableWidgetItem(op->getFormattedLocation());
        QTableWidgetItem *bytes   = new QTableWidgetItem(op->getDataString());
        QTableWidgetItem *opcode  = new QTableWidgetItem(op->getName());
        QTableWidgetItem *data    = new QTableWidgetItem(op->getArgsString());

        QColor funcCol(0, 12, 140);
        QFont funcFont("Roboto Mono Bold", 10);

        opcode->setForeground(QColor(48,  98, 174));
        bytes->setForeground(QColor(120, 120, 120));

        int index = m_disasm->rowCount();
        m_disasm->setRowCount(index + 1);

        if (m_script.getJumps().count(op->getLocation()) == 1)
        {
            QTableWidgetItem *jump = new QTableWidgetItem(QString(":" + m_script.getJumps().at(op->getLocation())));
            jump->setForeground(QColor(255, 0, 0));

            m_disasm->setItem(index++, 1, jump);
            m_disasm->insertRow(index);
        }

        if (op->getOp() == EOpcodes::OP_NATIVE)
        {
            int native   = ((op->getData()[0] << 2) & 0x300) | op->getData()[1];
            int argCount = (op->getData()[0] & 0x3e) >> 1;
            bool hasRets = (op->getData()[0] & 1) == 1 ? true : false;

            data->setText(QString("%1 (%2 args, ret %3)").arg(Util::getNative(m_script.getNatives()[native], m_nativeMap))
                                                         .arg(argCount)
                                                         .arg(hasRets));

        }
        else if (op->getOp() == EOpcodes::OP_ENTER)
        {
            if (m_script.getFuncNames().at(op->getLocation()) != "__entrypoint")
            {
                m_disasm->insertRow(index++);
            }

            data->setText(m_script.getFuncNames().at(op->getLocation()));

            address->setFont(funcFont);
            bytes->setFont(funcFont);
            opcode->setFont(funcFont);
            data->setFont(funcFont);

            address->setForeground(funcCol);
            bytes->setForeground(funcCol);
            opcode->setForeground(funcCol);
            data->setForeground(funcCol);

            // add function to func table
            int index = m_ui->funcTable->rowCount();

            m_ui->funcTable->setRowCount(index + 1);

            m_ui->funcTable->setItem(index, 0, new QTableWidgetItem(op->getFormattedLocation()));
            m_ui->funcTable->setItem(index, 1, new QTableWidgetItem(m_script.getFuncNames().at(op->getLocation())));
        }
        else if (op->getOp() >= EOpcodes::OP_CALL2 && op->getOp() <= EOpcodes::OP_CALL2HF)
        {
            QDataStream str(op->getData());

            unsigned short funcLoc;
            str >> funcLoc;

            int callOffset = (funcLoc | (op->getOp() - 0x52) << 0x10);

            callOffset += m_script.getPageOffsets()[m_script.getPageByLocation(callOffset)];

            if (m_script.getFuncNames().count(callOffset) == 0)
            {
                data->setText(QString("??? (%1)").arg(callOffset, 5, 16));
                invalidCalls++;
            }
            else
            {
                data->setText(m_script.getFuncNames().at(callOffset));
            }

            data->setFont(funcFont);
            data->setForeground(funcCol);
        }
        else if (op->getOp() >= EOpcodes::OP_JMP && op->getOp() <= EOpcodes::OP_JMPGT)
        {
            int jumpPos = op->getData()[1] + op->getLocation() + 3;
            data->setText("@" + m_script.getJumps().at(jumpPos));
            data->setForeground(QColor(255, 0, 0));
        }

        m_disasm->setItem(index, 0, address);
        m_disasm->setItem(index, 1, bytes);
        m_disasm->setItem(index, 2, opcode);
        m_disasm->setItem(index, 3, data);
    }

    if (invalidCalls > 0)
    {
        QMessageBox::warning(this, "Warning", QString("Warning: %1 invalid calls found.").arg(invalidCalls));
    }

    m_ui->funcTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
}
