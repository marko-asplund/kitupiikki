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
#include "pilviyhteys.h"

#include "db/kirjanpito.h"
#include <QSettings>

PilviYhteys::PilviYhteys(QObject *parent, int pilviId, QString osoite, QString token) :
    KpYhteys (parent ), pilviId_(pilviId), pilviosoite_(osoite), token_(token)
{

}

PilviKysely* PilviYhteys::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new PilviKysely(this, metodi, polku);
}

void PilviYhteys::alustaYhteys()
{
    PilviKysely *alustusKysely = kysely("/init");
    connect( alustusKysely, &PilviKysely::vastaus, this, &PilviYhteys::initSaapui );
    alustusKysely->kysy();
}

void PilviYhteys::initSaapui(QVariantMap * /* reply */, int tila)
{
    emit yhteysAvattu( tila == PilviKysely::OK );
    if( tila == PilviKysely::OK)
        kp()->settings()->setValue("Viimeisin", pilviId());
    sender()->deleteLater();
}