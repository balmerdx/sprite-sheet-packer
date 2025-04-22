#pragma once

#include "pugixml.hpp"
#include <QString>

pugi::xml_parse_result pugi_load_xml(pugi::xml_document& doc, const QString& filename);
bool pugi_save_xml(pugi::xml_document& doc, const QString& filename);

//По умолчанию пишут без форматирования текста
QString saveDocumentToString(const pugi::xml_document& document);
QString saveNodeToString(const pugi::xml_node& node, bool format_intent = false);

pugi::xml_parse_result readFromString(pugi::xml_document& document, const QString& content);
pugi::xml_parse_result readFromArray(pugi::xml_document& document, const QByteArray& bytesUtf8);
