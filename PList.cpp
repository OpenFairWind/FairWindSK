//
// Created by Raffaele Montella on 17/01/25.
//

#include "PList.hpp"

#include <QDomNode>
#include <QDomDocument>
#include <QDateTime>
#include <QDebug>


static QDomElement textElement(QDomDocument& doc, const char *tagName, QString contents) {
	QDomElement tag = doc.createElement(QString::fromLatin1(tagName));
	tag.appendChild(doc.createTextNode(contents));
	return tag;
}


PList::PList(QIODevice *device) {
	QVariantMap result;
	QDomDocument doc;
	QString errorMessage;
	int errorLine;
	int errorColumn;
	bool success = doc.setContent(device, false, &errorMessage, &errorLine, &errorColumn);
	if (!success) {
		qDebug() << "PListParser Warning: Could not parse PList file!";
		qDebug() << "Error message: " << errorMessage;
		qDebug() << "Error line: " << errorLine;
		qDebug() << "Error column: " << errorColumn;

	}

	QDomElement root = doc.documentElement();
	if (root.attribute(QStringLiteral("version"), QStringLiteral("1.0")) != QLatin1String("1.0")) {
		qDebug() << "PListParser Warning: plist is using an unknown format version, parsing might fail unexpectedly";
	}

	m_plist = parseElement(root.firstChild().toElement());
}

QVariant PList::parseElement(const QDomElement &e) {
	QString tagName = e.tagName();
	QVariant result;
	if (tagName == QLatin1String("dict")) {
		result = parseDictElement(e);
	}
	else if (tagName == QLatin1String("array")) {
		result = parseArrayElement(e);
	}
	else if (tagName == QLatin1String("string")) {
		result = e.text();
	}
	else if (tagName == QLatin1String("data")) {
		result = QByteArray::fromBase64(e.text().toUtf8());
	}
	else if (tagName == QLatin1String("integer")) {
		result = e.text().toInt();
	}
	else if (tagName == QLatin1String("real")) {
		result = e.text().toFloat();
	}
	else if (tagName == QLatin1String("true")) {
		result = true;
	}
	else if (tagName == QLatin1String("false")) {
		result = false;
	}
	else if (tagName == QLatin1String("date")) {
		result = QDateTime::fromString(e.text(), Qt::ISODate);
	}
	else {
		qDebug() << "PListParser Warning: Invalid tag found: " << e.tagName() << e.text();
	}
	return result;
}

QVariantList PList::parseArrayElement(const QDomElement& element) {
	QVariantList result;
	QDomNodeList children = element.childNodes();
	for (int i = 0; i < children.count(); i++) {
		QDomNode child = children.at(i);
		QDomElement e = child.toElement();
		if (!e.isNull()) {
			result.append(parseElement(e));
		}
	}
	return result;
}

QVariantMap PList::parseDictElement(const QDomElement& element) {
	QVariantMap result;
	QDomNodeList children = element.childNodes();
	QString currentKey;
	for (int i = 0; i < children.count(); i++) {
		QDomNode child = children.at(i);
		QDomElement e = child.toElement();
		if (!e.isNull()) {
			QString tagName = e.tagName();
			if (tagName == QLatin1String("key")) {
				currentKey = e.text();
			}
			else if (!currentKey.isEmpty()) {
				result[currentKey] = parseElement(e);
			}
		}
	}
	return result;
}

 QDomElement PList::serializePrimitive(QDomDocument &doc, const QVariant &variant) {
	QDomElement result;
	if (variant.typeId() == QMetaType::Type::Bool) {
        result = doc.createElement(variant.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
	}
	else if (variant.typeId() == QMetaType::Type::QDate) {
		result = textElement(doc, "date", variant.toDate().toString(Qt::ISODate));
	}
	else if (variant.typeId() == QMetaType::Type::QDateTime) {
		result = textElement(doc, "date", variant.toDateTime().toString(Qt::ISODate));
	}
	else if (variant.typeId() == QMetaType::Type::QByteArray) {
		result = textElement(doc, "data", QString::fromLatin1(variant.toByteArray().toBase64()));
	}
	else if (variant.typeId() == QMetaType::Type::QString) {
		result = textElement(doc, "string", variant.toString());
	}
	else if (variant.typeId() == QMetaType::Type::Int) {
		result = textElement(doc, "integer", QString::number(variant.toInt()));
	}
	else if (variant.canConvert<double>()) {
		QString num;
		num.setNum(variant.toDouble());
		result = textElement(doc, "real", num);
	}
	return result;
}

QDomElement PList::serializeElement(QDomDocument &doc, const QVariant &variant) {
	if (variant.typeId() == QMetaType::Type::QVariantMap) {
		return serializeMap(doc, variant.toMap());
	}
	else if (variant.typeId() == QMetaType::Type::QVariantList) {
		 return serializeList(doc, variant.toList());
	}
	else {
		return serializePrimitive(doc, variant);
	}
}

QDomElement PList::serializeList(QDomDocument &doc, const QVariantList &list) {
	QDomElement element = doc.createElement(QStringLiteral("array"));
	foreach(QVariant item, list) {
		element.appendChild(serializeElement(doc, item));
	}
	return element;
}

QDomElement PList::serializeMap(QDomDocument &doc, const QVariantMap &map) {
	QDomElement element = doc.createElement(QStringLiteral("dict"));
	QList<QString> keys = map.keys();
	foreach(QString key, keys) {
		QDomElement keyElement = textElement(doc, "key", key);
		element.appendChild(keyElement);
		element.appendChild(serializeElement(doc, map[key]));
	}
	return element;
}

QString PList::toPList(const QVariant &variant) {
	QDomDocument document(QStringLiteral("plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\""));
	document.appendChild(document.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
	QDomElement plist = document.createElement(QStringLiteral("plist"));
	plist.setAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
	document.appendChild(plist);
	plist.appendChild(serializeElement(document, variant));
	return document.toString();
}

QMap<QString, QVariant> PList::toMap()
{
	return m_plist.toMap();
}
