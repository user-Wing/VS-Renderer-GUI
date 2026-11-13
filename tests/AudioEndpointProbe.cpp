#include "backend/AudioEndpoints.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
int main(int argc,char **argv){QCoreApplication app(argc,argv);QJsonArray devices;for(const auto &device:vsr::audioEndpoints())devices.append(QJsonObject{{"id",device.id},{"name",device.name}});std::cout<<QJsonDocument(devices).toJson().constData();return 0;}
