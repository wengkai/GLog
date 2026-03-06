#include "GCommandLineParser.h"
#include <QCommandLineOption>

GCommandLineParser::GCommandLineParser()
{
    addHelpOption();
    addVersionOption();
    addOption(QCommandLineOption("i", "Input file(s)", "input-file"));
    addOption(QCommandLineOption("remove", "Remove file(s) after processing"));
}