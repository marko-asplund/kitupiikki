/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "pilvimodel.h"
#include "db/kirjanpito.h"
#include "pilviyhteys.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

PilviModel::PilviModel(QObject *parent) :
    QAbstractListModel (parent)
{

}

int PilviModel::rowCount(const QModelIndex & /* parent */) const
{
    return pilvet_.count();
}

QVariant PilviModel::data(const QModelIndex &index, int role) const
{
    QVariantMap map = pilvet_.at( index.row() );
    if( role == Qt::DisplayRole || role == NimiRooli)
    {        
        return map.value("name").toString();
    } else if( role == IdRooli ) {
        return map.value("id").toInt();
    }

    return QVariant();
}


QString PilviModel::pilviLoginOsoite()
{
    return "http://localhost:5665/api";
}

bool PilviModel::avaaPilvesta(int pilviId)
{
    for( auto map : pilvet_) {
        if( map.value("id").toInt() == pilviId) {
            PilviYhteys *yhteys = new PilviYhteys(this, pilviId, map.value("url").toString(),
                                   map.value("token").toString());
            connect( yhteys, &PilviYhteys::yhteysAvattu, kp(), &Kirjanpito::yhteysAvattu);
            yhteys->alustaYhteys();
            return true;
        }
    }
    return false;
}

void PilviModel::kirjaudu(const QString sahkoposti, const QString &salasana, bool pyydaAvain)
{
    QVariantMap map;
    if( !salasana.isEmpty()) {
        map.insert("email", sahkoposti);
        map.insert("password", salasana);
        if( pyydaAvain )
            map.insert("requestKey",true);
    } else if( kp()->settings()->contains("CloudKey") ) {
        map.insert("email", kp()->settings()->value("CloudEmail") );
        map.insert("key", kp()->settings()->value("CloudKey"));
    }

    QNetworkAccessManager *mng = kp()->networkManager();

    // Tähän pilviosoite!
    QNetworkRequest request(QUrl( pilviLoginOsoite() + "/login") );


    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent","Kitupiikki");

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &PilviModel::kirjautuminenValmis);

}

void PilviModel::kirjauduUlos()
{
    beginResetModel();
    pilvet_.clear();
    kp()->settings()->remove("CloudEmail");
    kp()->settings()->remove("CloudKey");

    kayttajaId_ = 0;
    kayttajaNimi_.clear();

    endResetModel();
}



void PilviModel::kirjautuminenValmis()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if( reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
        emit loginvirhe();
        return;
    }

    QByteArray vastaus = reply->readAll();


    QJsonDocument doc = QJsonDocument::fromJson( vastaus );

    kayttajaId_ = doc.object().value("id").toInt();
    kayttajaNimi_ = doc.object().value("name").toString();

    if( doc.object().contains("key"))
    {
        kp()->settings()->setValue("CloudKey", doc.object().value("key").toString());
        kp()->settings()->setValue("CloudEmail", doc.object().value("email").toString());
    }

    beginResetModel();
    pilvet_.clear();
    QVariantList lista = doc.object().value("clouds").toVariant().toList();
    for( auto item: lista ){
        pilvet_.append( item.toMap() );
    }
    endResetModel();

    if( kp()->settings()->value("Viimeisin").toInt() > 0)
        avaaPilvesta( kp()->settings()->value("Viimeisin").toInt() );

    emit kirjauduttu();
}
