#include "DownloadSource.h"

// DownloadSource constructors
DownloadSource::DownloadSource() : name(""), url(""), type(""), proxy(false) {}

DownloadSource::DownloadSource(const QString &name, const QString &url, const QString &type, bool proxy)
    : name(name), url(url), type(type), proxy(proxy) {}

// DownloadSource getters
QString DownloadSource::getName() const { return name; }
QString DownloadSource::getUrl() const { return url; }
QString DownloadSource::getType() const { return type; }
bool DownloadSource::isProxy() const { return proxy; }

// DownloadSource setters
void DownloadSource::setName(const QString &newName) { name = newName; }
void DownloadSource::setUrl(const QString &newUrl) { url = newUrl; }
void DownloadSource::setType(const QString &newType) { type = newType; }
void DownloadSource::setProxy(bool newProxy) { proxy = newProxy; }

// YggSource constructors
YggSource::YggSource() : name(""), url("") {}

YggSource::YggSource(const QString &name, const QString &url) : name(name), url(url) {}

// YggSource getters
QString YggSource::getName() const { return name; }
QString YggSource::getUrl() const { return url; }

// YggSource setters
void YggSource::setName(const QString &newName) { name = newName; }
void YggSource::setUrl(const QString &newUrl) { url = newUrl; }
