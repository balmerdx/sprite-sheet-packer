#include "pugi_utils.h"

pugi::xml_parse_result pugi_load_xml(pugi::xml_document& doc, const QString& filename)
{
    const int maxLen = 512;
    wchar_t wname[maxLen];
    int len = filename.toWCharArray(wname);
    Q_ASSERT(len+1<maxLen);
    wname[len] = 0;
    return doc.load_file(wname);
}

bool pugi_save_xml(pugi::xml_document& doc, const QString& filename)
{
    /*
    //Явно добавляем utf-8
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version").set_value("1.0");
    decl.append_attribute("encoding").set_value("utf-8");
    */

    const int maxLen = 512;
    wchar_t wname[maxLen];
    int len = filename.toWCharArray(wname);
    Q_ASSERT(len+1<maxLen);
    wname[len] = 0;
    return doc.save_file(wname);
}

class StringXmlWriter: public pugi::xml_writer
{
public:
    void write(const void* data, size_t size) override
    {
        _buffer += QString::fromUtf8(static_cast<const char*>(data), static_cast<int>(size));
    }

    const QString& buffer() const { return _buffer;}
protected:
    QString _buffer;
};

QString saveDocumentToString(const pugi::xml_document& document)
{
    StringXmlWriter writer;
    document.save(writer, "\t", pugi::format_raw| pugi::format_no_declaration);
    return writer.buffer();
}

QString saveNodeToString(const pugi::xml_node& node, bool format_intent)
{
    StringXmlWriter writer;
    node.print(writer, "\t", (format_intent ? pugi::format_indent : pugi::format_raw)
               | pugi::format_no_declaration);
    return writer.buffer();
}

pugi::xml_parse_result readFromString(pugi::xml_document& document, const QString& content)
{
    return document.load_buffer(content.utf16(), static_cast<size_t>(content.size()*2),
                           pugi::parse_default, pugi::encoding_utf16 );
}

pugi::xml_parse_result readFromArray(pugi::xml_document& document, const QByteArray& bytesUtf8)
{
    return document.load_buffer(bytesUtf8.data(), static_cast<size_t>(bytesUtf8.size()),
                           pugi::parse_default, pugi::encoding_utf8 );
}
