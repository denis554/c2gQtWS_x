#include "datautil.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QImage>

const QString YYYY_MM_DD = "yyyy-MM-dd";
const QString HH_MM = "HH:mm";
const QString LOCAL_HH_MM = "hh:mm";
const QString YYYY_MM_DD_HH_MM = "yyyy-MM-ddHH:mm";
const QString DAYNAME = "dddd";
const QString DAYNAME_HH_MM = "dddd, HH:mm";
const QString DEFAULT_SPEAKER_IMAGE_URL = "https://s3-eu-west-1.amazonaws.com/qt-worldsummit/ws2016/uploads/2016/07/man-silhouette-black-gray.jpg";
//"http://conf.qtcon.org/person_original.png";
const QString EMPTY_TRACK = "*****";

DataUtil::DataUtil(QObject *parent) : QObject(parent)
{

}

void DataUtil::init(DataManager* dataManager, DataServer* dataServer)
{
    mDataManager = dataManager;
    mDataServer = dataServer;
    mConferenceDataPath = mDataManager->mDataPath + "conference/";
    mCacheDataPath = mDataManager->mDataPath + "prod/";
    mDataServer->setConferenceDataPath(mConferenceDataPath);
    // used for temp dynamic lists as QQmlPropertyLists
    mSessionLists = mDataManager->createSessionLists();

    mCurrentConference = nullptr;

    // connections
    bool res = connect(mDataServer, SIGNAL(serverSuccess()), this,
                       SLOT(onServerSuccess()));
    if (!res) {
        Q_ASSERT(res);
    }
    res = connect(mDataServer, SIGNAL(serverFailed(QString)), this,
                  SLOT(onServerFailed(QString)));
    if (!res) {
        Q_ASSERT(res);
    }

    res = connect(mDataServer, SIGNAL(versionSuccess(QByteArray)), this,
                  SLOT(onVersionSuccess(QByteArray)));
    if (!res) {
        Q_ASSERT(res);
    }
    res = connect(mDataServer, SIGNAL(versionFailed(QString)), this,
                  SLOT(onVersionFailed(QString)));
    if (!res) {
        Q_ASSERT(res);
    }
}

QString DataUtil::conferenceDataPath4QML() {

#if defined (Q_OS_WIN)
    return "file:///"+mConferenceDataPath;
#else
    return "file://"+mConferenceDataPath;
#endif
}

bool DataUtil::isNoConference()
{
    if(mDataManager->allConference().empty()) {
        qDebug() << "cpp: isNoConference true";
        return true;
    }
    qDebug() << "cpp: we have conferences: " << mDataManager->allConference().size();
    return false;
    
}

bool DataUtil::isOldConference()
{
    if(!mDataManager->allConference().empty()) {
        Conference* conference = static_cast<Conference*>( mDataManager->allConference().last());
        if(conference->id() < 201801) {
            qDebug() << " we have old conference data";
            return true;
        }
    }
    return false;
}

bool DataUtil::isDateTooLate()
{
    if(mDataManager->allDay().empty()) {
        return true;
    }
    QString todayDate = QDate::currentDate().toString(YYYY_MM_DD);
    QString lastConferenceDay = (static_cast<Day*>( mDataManager->allDay().last()))->conferenceDay().toString(YYYY_MM_DD);
    qDebug() << "todayDate" << todayDate << "lastConferenceDay" << lastConferenceDay;
    return todayDate > lastConferenceDay;
}

/**
 * @brief DataUtil::sessionInfoForSpeaker
 * @param speaker
 * @return all Session Title line by line
 */
QString DataUtil::sessionInfoForSpeaker(Speaker *speaker)
{
    QString info;
    if(!speaker) {
        return info;
    }
    for (int i = 0; i < speaker->sessions().size(); ++i) {
        if(i > 0) {
            info.append("\n");
        }
        info.append(speaker->sessions().at(i)->title());
    }
    return info;
}

QString DataUtil::speakerNamesForSession(Session *session)
{
    QString info;
    if(!session) {
        return info;
    }
    for (int i = 0; i < session->presenter().size(); ++i) {
        QString name = session->presenter().at(i)->name();
        QString company = session->presenter().at(i)->title();
        if(name.length()) {
            if(info.length()) {
                info.append("; ");
            }
            info.append(name);
            if(company.length()) {
                info.append(", ");
                info.append(company);
            }
        }
    }
    return info;
}

QString DataUtil::scheduleItemImageForSession(Session *session)
{
    if(!session || !session->isGenericScheduleSession()) {
        return "";
    }
    if(session->isRegistration()) {
        return "key.png";
    }
    if(session->isLunch()) {
        return "lunch.png";
    }
    if(session->isEvent()) {
        return "party_event.png";
    }
    return "break.png";
}

QString DataUtil::letterForButton(Session *session)
{
    if(!session) {
        return "S";
    }
    if(session->isTraining()) {
        return "T";
    }
    if(session->isLightning()) {
        return "L";
    }
    if (session->isKeynote()) {
        return "K";
    }
    if(session->isCommunity()) {
        return "C";
    }
    if(session->isMeeting()) {
        return "M";
    }
    if(session->isUnconference()) {
        return "U";
    }
    return "S";
}

QString DataUtil::textForSessionTrack(SessionTrack *sessionTrack)
{
    if(sessionTrack->name() == "*****") {
        return "";
    }
    return sessionTrack->name();
}

QString DataUtil::textForSessionType(Session *session)
{
    if(!session) {
        return "";
    }
    QString info = " (" + QString::number(session->minutes()) + tr(" Minutes)");
    if(session->isGenericScheduleSession()) {
        if(session->isRegistration()) {
            return tr("Registration")+info;
        }
        if(session->isEvent()) {
            return tr("Event")+info;
        }
        if(session->isLunch()) {
            return tr("Lunch")+info;
        }
        return tr("Break")+info;
    }
    if(session->isTraining()) {
        return tr("Training")+info;
    }
    if(session->isLightning()) {
        return tr("Lightning Talk")+info;
    }
    if(session->isKeynote()) {
        return tr("Keynote")+info;
    }
    if(session->isCommunity()) {
        return tr("Community")+info;
    }
    if(session->isUnconference()) {
        return tr("Unconference")+info;
    }
    if(session->isMeeting()) {
        return tr("Meeting")+info;
    }
    return tr("Session")+info;
}

QString DataUtil::trackColorFirstTrack(Session *session)
{
    if(session->sessionTracks().empty()) {
        return "transparent";
    }
    return session->sessionTracks().first()->color();
}

QString DataUtil::displayStartToEnd(Session *session)
{
    QString fromTo = "";
    if(!session) {
        qWarning() << "uuups displayStartToEnd - no Session Object ";
        return fromTo;
    }
    if(session->startTime().isValid()) {
        fromTo = session->startTime().toString(HH_MM);
    } else {
        qWarning() << "uuups displayStartToEnd - start time not valid ";
    }
    fromTo.append(" - ");
    if(session->endTime().isValid()) {
        fromTo.append(session->endTime().toString(HH_MM));
    } else {
        qWarning() << "uuups displayStartToEnd - end time not valid ";
    }
    // qDebug() << "display fromTo: " << fromTo;
    return fromTo;
}

// if update failed Data in memory is inconsistent
// delete all, then do init again
void DataUtil::reloadData() {
    // delete all to avoid orphans
    mProgressInfotext.append(tr(" Rollback"));
    emit progressInfo(mProgressInfotext);
    // DO IT
    // reload
    mProgressInfotext.append(tr(" Reload"));
    emit progressInfo(mProgressInfotext);
    // DO IT
    mDataManager->init();
}

// creates missing dirs if preparing conference (pre-conf-stuff)
// or checking for schedule updates
bool DataUtil::checkDirs()
{
    // data/conference
    QString directory = mDataManager->mDataPath + "conference/";
    QDir myDir;
    bool exists;
    exists = myDir.exists(directory);
    if (!exists) {
        bool ok = myDir.mkpath(directory);
        if(!ok) {
            qWarning() << "Couldn't create conference dir " << directory;
            return false;
        }
        qDebug() << "created directory conference " << directory;
    }
    // data/conference/speakerImages
    directory.append("speakerImages/");
    exists = myDir.exists(directory);
    if (!exists) {
        bool ok = myDir.mkpath(directory);
        if(!ok) {
            qWarning() << "Couldn't create speakerImages dir " << directory;
            return false;
        }
        qDebug() << "created directory speakerImages " << directory;
    }
    return true;
}

// Conference, Days, Rooms
// some special stuff to initialize a new conference
// or to update if there were changes ion data-assets
void DataUtil::prepareConference() {
    qDebug() << "PREPARE CONFERENCE ";
    // check dirs for pre-conference stuff
    bool dirsOk = checkDirs();
    if(!dirsOk) {
        qWarning() << "cannot create directories";
        return;
    }
    // create some data for this specific conference
    prepareEventData();

    qDebug() << "cache DATA";
    mDataManager->saveConferenceToCache();
    mDataManager->saveDayToCache();
    mDataManager->saveRoomToCache();

    qDebug() << "PREPARE   D O N E";
}

// conference, days, rooms
void DataUtil::prepareEventData() {
    qDebug() << "PREPARE EVENT ";
    // CONFERENCE
    mDataManager->deleteConference();
    mDataManager->deleteRoom();
    mDataManager->deleteDay();
    mDataManager->deleteSession();
    mDataManager->deleteSessionTrack();
    // at first we delete current Speaker and SpeakerImages file
    // perhaps from old conferences
    mDataManager->deleteSpeaker();
    mDataManager->deleteSpeakerImage();
    QString cacheSpeakerImageFilePath = mCacheDataPath + "cacheSpeakerImage.json";
    QFile cacheSpeakerImageFile(cacheSpeakerImageFilePath);
    if(cacheSpeakerImageFile.exists()) {
        bool ok = cacheSpeakerImageFile.remove();
        qDebug() << "old speaker images removed from cache? " << ok;
    }
    QString cacheSpeakerFilePath = mCacheDataPath + "cacheSpeaker.json";
    QFile cacheSpeakerFile(cacheSpeakerFilePath);
    if(cacheSpeakerFile.exists()) {
        bool ok = cacheSpeakerFile.remove();
        qDebug() << "old speakers removed from cache? " << ok;
    }
    // as next we copy speaker and speaker images from assets
    mDataManager->initSpeakerImageFromCache();
    mDataManager->initSpeakerFromCache();
    qDebug() << "copied speakers: " << mDataManager->allSpeaker().size();
    qDebug() << "copied speaker images: " << mDataManager->allSpeakerImage().size();
    mDataManager->resolveReferencesForAllSpeaker();
    // Rooms and Room Images
    prepareRooms();
    // Conference, Days, connect Rooms
    prepareBoston201801();
    prepareBerlin201802();
}

// Rooms
void DataUtil::prepareRooms() {
    // create dummi room if session with no room name from update
    Room* room = mDataManager->createRoom();
    room->setRoomId(0);
    room->setRoomName(tr("Room unknown"));
    // we have no image
    room-> setInAssets(false);
    mDataManager->insertRoom(room);
    const QString path = ":/data-assets/conference/roomimages/mapping.json";
    qDebug() << "PREPARE ROOMS ";
    QVariantList dataList;
    dataList = readRoomMappingFile(path);
    qDebug() << "read room mappings #" << dataList.size();
    for (int i = 0; i < dataList.size(); ++i) {
        QVariantMap map = dataList.at(i).toMap();
        int id = map.value("id").toInt();
        QString name = map.value("name").toString();
        Room* room = mDataManager->createRoom();
        room->setRoomId(id);
        if(room->roomId() > 2018000 && room->roomId() < 2018100) {
            room->setConference(201801);
        } else if (room->roomId() > 2018100 && room->roomId() < 2018200) {
            room->setConference(201802);
        }
        room->setRoomName(name);
        room-> setInAssets(true);
        mDataManager->insertRoom(room);
    }
}

// Conference, Days
void DataUtil::prepareSanFrancisco201601() {
    Conference* conference = mDataManager->createConference();
    conference->setConferenceName("Qt World Summit 2016");
    conference->setConferenceCity("San Francisco, CA");
    QString venueAddress;
    venueAddress = "Pier 27";
    venueAddress.append("\n");
    venueAddress.append("San Francisco Cruise Ship Terminal");
    venueAddress.append("\n");
    venueAddress.append("The Embarcadero");
    venueAddress.append("\n");
    venueAddress.append("San Francisco, CA 94111");
    venueAddress.append("\n");
    venueAddress.append("United States");
    conference->setAddress(venueAddress);
    conference->setTimeZoneName("PST – Pacific Standard Time");
    conference->setTimeZoneOffsetSeconds(-8 * 60 * 60); // -08:00 GMT
    conference->setConferenceFrom(QDate::fromString("2016-10-19", YYYY_MM_DD));
    conference->setConferenceTo(QDate::fromString("2016-10-20", YYYY_MM_DD));
    conference->setHashTag("#QtWS16");
    conference->setHomePage("http://www.qtworldsummit.com/");
    QString coordinate;
    coordinate = QString::number(37.799263)+","+QString::number(-122.397673);
    conference->setCoordinate(coordinate);
    mDataManager->insertConference(conference);
    // DAYS
    // Days dayOfWeek 1=monday, 7 = sunday
    // wednesday
    Day* day = mDataManager->createDay();
    day->setId(1);
    day->setConference(201601);
    day->setWeekDay(3);
    day->setConferenceDay(QDate::fromString("2016-10-19", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    // thursday
    day = mDataManager->createDay();
    day->setId(2);
    day->setWeekDay(4);
    day->setConference(201601);
    day->setConferenceDay(QDate::fromString("2016-10-20", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    qDebug() << "CONFERENCE " << conference->conferenceCity() << " with days #" << conference->days().size();
    conference->setLastSessionTrackId(conference->id()*100);
    conference->setLastGenericSessionId(conference->id()*100*-1);
    qDebug() << "Last SessionTrack " << conference->lastSessionTrackId() << " last Generic Session ID " << conference->lastGenericSessionId();
    // rooms
    for (int i = 0; i < mDataManager->allRoom().size(); ++i) {
        Room* room = static_cast<Room*>( mDataManager->allRoom().at(i));
        if(room->conference() == 201601) {
            conference->addToRooms(room);
            if(room->roomId() > conference->lastRoomId()) {
                conference->setLastRoomId(room->roomId());
            }
        }
    }
    // ekke TODO fix generator bug (without resolving we get days, rooms tracks doubled)
    QList<Day*> dList;
    conference->resolveDaysKeys(dList);
    QList<Room*> rList;
    conference->resolveRoomsKeys(rList);
}

// Conference, Days
void DataUtil::prepareBoston201801() {
    qDebug() << "prepareBoston201801";
    Conference* conference = mDataManager->createConference();
    conference->setId(201801);
    conference->setConferenceName("Qt World Summit 2018");
    conference->setConferenceCity("Boston, MA");
    QString venueAddress;
    venueAddress = "425 Summer Street";
    venueAddress.append("\n");
    venueAddress.append("The Westin Boston Waterfront");
    venueAddress.append("\n");
    venueAddress.append("Boston, Massachusetts 02210");
    venueAddress.append("\n");
    venueAddress.append("United States");
    conference->setAddress(venueAddress);
    conference->setMapAddress("425%20Summer%20Street,The%20Westin%20Boston%20Waterfront,Boston,Massachusetts");
    conference->setTimeZoneName("EST – Eastern Daylight Time");
    conference->setTimeZoneOffsetSeconds(-4 * 60 * 60); // -04:00 GMT
    conference->setConferenceFrom(QDate::fromString("2018-10-29", YYYY_MM_DD));
    conference->setConferenceTo(QDate::fromString("2018-10-30", YYYY_MM_DD));
    conference->setHashTag("#QtWS18");
    conference->setHomePage("https://www.qt.io/qtws18/home/");
    QString coordinate;
    coordinate = QString::number(42.3459926)+","+QString::number(-71.04301040000001);
    conference->setCoordinate(coordinate);
    conference->setPlaceId("ChIJde0H54N644kR8QDFjYVOlMU");
    mDataManager->insertConference(conference);
    // DAYS
    // Days dayOfWeek 1=monday, 7 = sunday
    // monday
    Day* day = mDataManager->createDay();
    day->setId(2018011);
    day->setWeekDay(1);
    day->setConference(201801);
    day->setConferenceDay(QDate::fromString("2018-10-29", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    // tuesday
    day = mDataManager->createDay();
    day->setId(2018012);
    day->setConference(201801);
    day->setWeekDay(2);
    day->setConferenceDay(QDate::fromString("2018-10-30", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    qDebug() << "CONFERENCE " << conference->conferenceCity() << " with days #" << conference->days().size();
    conference->setLastSessionTrackId(conference->id()*100);
    conference->setLastGenericSessionId(conference->id()*100*-1);
    qDebug() << "Last SessionTrack " << conference->lastSessionTrackId() << " last Generic Session ID " << conference->lastGenericSessionId();
    // rooms
    for (int i = 0; i < mDataManager->allRoom().size(); ++i) {
        Room* room = static_cast<Room*>( mDataManager->allRoom().at(i));
        if(room->conference() == 201801) {
            conference->addToRooms(room);
            if(room->roomId() > conference->lastRoomId()) {
                conference->setLastRoomId(room->roomId());
            }
        }
    }
    // ekke TODO fix generator bug (without resolving we get days, rooms tracks doubled)
    QList<Day*> dList;
    conference->resolveDaysKeys(dList);
    QList<Room*> rList;
    conference->resolveRoomsKeys(rList);
}

// Conference, Days
void DataUtil::prepareBerlin201802() {
    qDebug() << "prepareBerlin201802";
    Conference* conference = mDataManager->createConference();
    conference->setId(201802);
    conference->setConferenceName("Qt World Summit 2018");
    conference->setConferenceCity("Berlin");
    QString venueAddress;
    venueAddress = "Alexanderstraße 11";
    venueAddress.append("\n");
    venueAddress.append("bcc Berlin Congress Center");
    venueAddress.append("\n");
    venueAddress.append("10178 Berlin");
    venueAddress.append("\n");
    venueAddress.append("Germany");
    conference->setAddress(venueAddress);
    conference->setMapAddress("Alexanderstraße%2011,bcc%20Berlin%20Congress%20Center,Berlin,Germany");
    conference->setTimeZoneName("MEZ");
    conference->setTimeZoneOffsetSeconds(+1 * 60 * 60); // +01:00 GMT
    conference->setConferenceFrom(QDate::fromString("2018-12-05", YYYY_MM_DD));
    conference->setConferenceTo(QDate::fromString("2018-12-06", YYYY_MM_DD));
    conference->setHashTag("#QtWS18");
    conference->setHomePage("https://www.qt.io/qtws18/home/");
    QString coordinate;
    coordinate = QString::number(52.52043099999999)+","+QString::number(13.416334000000006);
    conference->setCoordinate(coordinate);
    conference->setPlaceId("ChIJQ7_AmBhOqEcRgCAfJCBSodI");
    mDataManager->insertConference(conference);
    // DAYS
    // Days dayOfWeek 1=monday, 7 = sunday
    // wednesday
    Day* day = mDataManager->createDay();
    day->setId(2018021);
    day->setConference(201802);
    day->setWeekDay(3);
    day->setConferenceDay(QDate::fromString("2018-12-05", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    // thursday
    day = mDataManager->createDay();
    day->setId(2018022);
    day->setConference(201802);
    day->setWeekDay(2);
    day->setConferenceDay(QDate::fromString("2018-12-06", YYYY_MM_DD));
    conference->addToDays(day);
    mDataManager->insertDay(day);
    qDebug() << "CONFERENCE " << conference->conferenceCity() << " with days #" << conference->days().size();
    conference->setLastSessionTrackId(conference->id()*100);
    conference->setLastGenericSessionId(conference->id()*100*-1);
    qDebug() << "Last SessionTrack " << conference->lastSessionTrackId() << " last Generic Session ID " << conference->lastGenericSessionId();
    // rooms
    for (int i = 0; i < mDataManager->allRoom().size(); ++i) {
        Room* room = static_cast<Room*>( mDataManager->allRoom().at(i));
        if(room->conference() == 201802) {
            conference->addToRooms(room);
            if(room->roomId() > conference->lastRoomId()) {
                conference->setLastRoomId(room->roomId());
            }
        }
    }
    // ekke TODO fix generator bug (without resolving we get days, rooms tracks doubled)
    QList<Day*> dList;
    conference->resolveDaysKeys(dList);
    QList<Room*> rList;
    conference->resolveRoomsKeys(rList);
}

QVariantMap DataUtil::readScheduleFile(const QString schedulePath) {
    QVariantMap map;
    QFile readFile(schedulePath);
    if(!readFile.exists()) {
        qWarning() << "Schedule Path not found " << schedulePath;
        return map;
    }
    if (!readFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file: " << schedulePath;
        return map;
    }
    QJsonDocument jda = QJsonDocument::fromJson(readFile.readAll());

    readFile.close();
    if(!jda.isObject()) {
        qWarning() << "Couldn't create JSON from file: " << schedulePath;
        return map;
    }
    qDebug() << "QJsonDocument for schedule with Object :)";
    map = jda.toVariant().toMap();
    return map;
}

Day* DataUtil::findDayForServerDate(const QString& dayDate, Conference* conference) {
    Day* day = nullptr;
    bool found = false;
    for (int dl = 0; dl < conference->days().size(); ++dl) {
        day = static_cast<Day*> (conference->days().at(dl));
        if(day->conferenceDay().toString(YYYY_MM_DD) == dayDate) {
            found = true;
            break;
        }
    }
    if(found) {
        qDebug() << "Day found";
        return day;
    }
    qDebug() << "Day not found for conference " << conference->conferenceCity();
    return nullptr;
}

void DataUtil::adjustTracks(QVariantMap& sessionMap, Conference* conference, const bool isUpdate) {
    QStringList trackKeys;
    QVariantList trackList;
    trackList = sessionMap.value("tracks").toList();
    for (int tnl = 0; tnl < trackList.size(); ++tnl) {
        QVariantMap trackMap;
        trackMap = trackList.at(tnl).toMap();
        QString trackName;
        trackName = trackMap.value("name").toString();
        QString trackColor;
        trackColor = trackMap.value("color").toString();
        bool found = false;
        for (int i = 0; i < conference->tracks().size(); ++i) {
            SessionTrack* sessionTrack = conference->tracks().at(i);
            if(sessionTrack->name() == trackName) {
                found = true;
                trackKeys.append(QString::number(sessionTrack->trackId()));
                break;
            }
        } // for all tracks from selected conference
        if(!found) {
            SessionTrack* sessionTrack = mDataManager->createSessionTrack();
            conference->setLastSessionTrackId(conference->lastSessionTrackId()+1);
            sessionTrack->setTrackId(conference->lastSessionTrackId());
            sessionTrack->setName(trackName);
            sessionTrack->setConference(conference->id());
            sessionTrack->setColor(trackColor);
            sessionTrack->setInAssets(!isUpdate);
            mDataManager->insertSessionTrack(sessionTrack);
            conference->addToTracks(sessionTrack);
            trackKeys.append(QString::number(sessionTrack->trackId()));
        }
    }
    sessionMap.insert("tracks", trackKeys);
    qDebug() << "TRACK KEYS: " << trackKeys;
}

void DataUtil::adjustPersons(QVariantMap& sessionMap) {
    QStringList personKeys;
    QVariantList personsList;
    personsList = sessionMap.value("persons").toList();
    if (!personsList.empty()) {
        for (int pvl = 0; pvl < personsList.size(); ++pvl) {
            QVariantMap map = personsList.at(pvl).toMap();
            if(map.contains("id")) {
                personKeys.append(map.value("id").toString());
            }
        }
        sessionMap.insert("persons", personKeys);
    }
}

bool DataUtil::checkIfIgnored(SessionAPI* sessionAPI) {
    Q_UNUSED(sessionAPI);
    return false;
    //    if(sessionAPI->title() == "Registration and Coffee" && sessionAPI->room() != "B02") {
    //        qDebug() << "unwanted session: " << sessionAPI->sessionId() << " " << sessionAPI->title() << " " << sessionAPI->room();
    //        return true;
    //    }
    //    if(sessionAPI->title() == "Lunch" && sessionAPI->room() != "B02") {
    //        qDebug() << "unwanted session: " << sessionAPI->sessionId() << " " << sessionAPI->title() << " " << sessionAPI->room();
    //        return true;
    //    }
    //    if(sessionAPI->title() == "Coffee break" && sessionAPI->room() != "B02") {
    //        qDebug() << "unwanted session: " << sessionAPI->sessionId() << " " << sessionAPI->title() << " " << sessionAPI->room();
    //        return true;
    //    }
    //    if(sessionAPI->title() == "Evening event" && sessionAPI->room() != "B02") {
    //        qDebug() << "unwanted session: " << sessionAPI->sessionId() << " " << sessionAPI->title() << " " << sessionAPI->room();
    //        return true;
    //    }
    //    if(sessionAPI->title() == "Welcome" && sessionAPI->room() != "C01") {
    //        qDebug() << "unwanted session: " << sessionAPI->sessionId() << " " << sessionAPI->title() << " " << sessionAPI->room();
    //        return true;
    //    }
    //    return false;
}

void DataUtil::setDuration(SessionAPI* sessionAPI, Session* session) {
    QStringList duration;
    duration = sessionAPI->duration().split(":");
    int minutes = 0;
    if(duration.length() == 2) {
        minutes = duration.last().toInt();
        minutes += duration.first().toInt()*60;
    } else {
        qWarning() << "Duration wrong: " << sessionAPI->duration() << " session ID: " << sessionAPI->sessionId();
    }
    session->setMinutes(minutes);
    session->setEndTime(session->startTime().addSecs(minutes * 60));
}

void DataUtil::setType(Session* session) {
    //    for (int i = 0; i < sessionAPI->tracksStringList().size(); ++i) {
    //        SessionTrack* sessionTrack = nullptr;
    //        bool found = false;
    //        QString trackName;
    //        trackName = sessionAPI->tracksStringList().at(i);
    //        if(trackName.isEmpty()) {
    //            trackName = EMPTY_TRACK;
    //        }
    //        for (int tr = 0; tr < mDataManager->mAllSessionTrack.size(); ++tr) {
    //            sessionTrack = (SessionTrack*) mDataManager->mAllSessionTrack.at(tr);
    //            if(sessionTrack->name() == trackName) {
    //                found = true;
    //                break;
    //            }
    //        }
    //        if(!found) {
    //            sessionTrack = mDataManager->createSessionTrack();
    //            conference->setLastSessionTrackId(conference->lastSessionTrackId()+1);
    //            sessionTrack->setTrackId(conference->lastSessionTrackId());
    //            sessionTrack->setName(trackName);
    //            sessionTrack->setInAssets(isUpdate?false:true);
    //            mDataManager->insertSessionTrack(sessionTrack);
    //        }
    //        if (sessionTrack) {
    //            session->sessionTracksKeys().append(QString::number(sessionTrack->trackId()));
    //        }
    //    }
    session->resolveSessionTracksKeys(mDataManager->listOfSessionTrackForKeys(session->sessionTracksKeys()));
    for (int i = 0; i < session->sessionTracks().size(); ++i) {
        SessionTrack* sessionTrack = session->sessionTracks().at(i);
        if (sessionTrack->name() == "Keynote") {
            session->setIsKeynote(true);
            return;
        }
    }
    // for QtCon here we create ScheduleItems which are not part of QtWS data
}

// sessions must be cleared before for Day Track, Room, Speaker
// always if prepare Conference, explicitely if Update Schedule
void DataUtil::sortedSessionsIntoRoomDayTrackSpeaker() {
    QMapIterator<QString, Session*> sessionIterator(mMultiSession);
    while (sessionIterator.hasNext()) {
        sessionIterator.next();
        Session* session = sessionIterator.value();
        mDataManager->insertSession(session);
        Room* room = mDataManager->findRoomByRoomId(session->room());
        if(room != nullptr) {
            room->addToSessions(session);
        } else {
            qWarning() << "ROOM is NULL for Session " << session->sessionId() << " #:" << session->room();
        }
        Day* day = mDataManager->findDayById(session->sessionDay());
        if(day != nullptr) {
            day->addToSessions(session);
        } else {
            qWarning() << "DAY is NULL for Session " << session->sessionId() << " #:" << session->sessionDay();
        }
        for (int i = 0; i < session->sessionTracksKeys().size(); ++i) {
            int tKey = session->sessionTracksKeys().at(i).toInt();
            SessionTrack* sessionTrack = static_cast<SessionTrack*> (mDataManager->findSessionTrackByTrackId(tKey));
            if(sessionTrack != nullptr) {
                sessionTrack->addToSessions(session);
            } else {
                qWarning() << "TRACK is NULL for Session " << session->sessionId() << " #:" << tKey;
            }
        }
        for (int i = 0; i < session->presenterKeys().size(); ++i) {
            int pKey = session->presenterKeys().at(i).toInt();
            Speaker* speaker = static_cast<Speaker*> (mDataManager->findSpeakerBySpeakerId(pKey));
            if(speaker != nullptr) {
                speaker->addToSessions(session);
            } else {
                qWarning() << "SPEAKER is NULL for Session " << session->sessionId() << " #:" << pKey;
            }
        } // for presenter
    } // while all sessions
}

QVariantList DataUtil::readSpeakerFile(const QString speakerPath) {
    QVariantList dataList;
    QFile readFile(speakerPath);
    if(!readFile.exists()) {
        qWarning() << "Speaker Path not found " << speakerPath;
        return dataList;
    }
    if (!readFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file: " << speakerPath;
        return dataList;
    }
    QJsonDocument jda = QJsonDocument::fromJson(readFile.readAll());

    readFile.close();
    if(!jda.isArray()) {
        qWarning() << "Couldn't create JSON from file: " << speakerPath;
        return dataList;
    }
    qDebug() << "QJsonDocument for speaker with Array :)";
    dataList = jda.toVariant().toList();
    return dataList;
}

QVariantList DataUtil::readRoomMappingFile(const QString path) {
    QVariantList dataList;
    QFile readFile(path);
    if(!readFile.exists()) {
        qWarning() << "Room Mapping Path not found " << path;
        return dataList;
    }
    if (!readFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file: " << path;
        return dataList;
    }
    QJsonDocument jda = QJsonDocument::fromJson(readFile.readAll());

    readFile.close();
    if(!jda.isArray()) {
        qWarning() << "Couldn't create JSON from file: " << path;
        return dataList;
    }
    qDebug() << "QJsonDocument for room mappings with Array :)";
    dataList = jda.toVariant().toList();
    return dataList;
}

void DataUtil::calcSpeakerName(Speaker* speaker, SpeakerAPI* speakerAPI) {
    speaker->setName(speakerAPI->firstName());
    if(speaker->name().length() > 0) {
        speaker->setName(speaker->name()+" ");
    }
    speaker->setName(speaker->name()+speakerAPI->lastName());
    if(speaker->name().length() > 0) {
        if(speakerAPI->lastName().length() > 0) {
            speaker->setSortKey(speakerAPI->lastName().left(5).toUpper());
        } else {
            speaker->setSortKey(speakerAPI->firstName().left(5).toUpper());
        }
        speaker->setSortGroup(speaker->sortKey().left(1));
    } else {
        speaker->setSortKey("*");
        speaker->setSortGroup("*");
    }
}

void DataUtil::checkVersion()
{
    mDataServer->requestVersion();
}

void DataUtil::startUpdate()
{
    bool dirOk = checkDirs();
    if(!dirOk) {
        qWarning() << "Cannot create Directories";
        emit updateFailed(tr("startUpdate - Cannot create Directories"));
        return;
    }
    mProgressInfotext = tr("Request Schedule and Speakers from Server");
    emit progressInfo(mProgressInfotext);
    mDataServer->requestSchedule(201801);
}

void DataUtil::continueUpdate()
{
    mProgressInfotext = tr("Save Favorites");
    emit progressInfo(mProgressInfotext);
    // save F A V O R I T E S
    saveSessionFavorites();
    // S P E A K E R
    mProgressInfotext = tr("Sync Speaker");
    emit progressInfo(mProgressInfotext);
    const QString speakersPath = mConferenceDataPath + "speaker.json";
    qDebug() << "PREPARE SPEAKER ";
    QVariantList dataList;
    dataList = readSpeakerFile(speakersPath);
    if(dataList.empty()) {
        qWarning() << "Speaker List empty";
        emit updateFailed(tr("Update failed. No Speaker received.\nReloading current Data"));
        return;
    }
    qDebug() << "we got speakers from server API #" << dataList.size();
    
    mMultiSession.clear();
    mMultiSpeaker.clear();
    mMultiSpeakerImages.clear();
    for (int i = 0; i < dataList.size(); ++i) {
        SpeakerAPI* speakerAPI = mDataManager->createSpeakerAPI();
        speakerAPI->fillFromForeignMap(dataList.at(i).toMap());

        Speaker* speaker = mDataManager->findSpeakerBySpeakerId(speakerAPI->id());
        if(!speaker) {
            // NEW speaker
            qDebug() << "NEW SPEAKER";
            mProgressInfotext.append("+");
            speaker = mDataManager->createSpeaker();
            speaker->setSpeakerId(speakerAPI->id());
        } else {
            // update Speaker
            qDebug() << "UPDATE SPEAKER";
            mProgressInfotext.append(".");
        }
        emit progressInfo(mProgressInfotext);
        calcSpeakerName(speaker, speakerAPI);
        speaker->setBio(speakerAPI->bio());
        speaker->setTitle(speakerAPI->title());
        if(speakerAPI->avatar().length() > 0 && speakerAPI->avatar() != DEFAULT_SPEAKER_IMAGE_URL && speakerAPI->avatar() != "false") {
            qDebug() << "Speaker has Avatar";
            QString avatar = speakerAPI->avatar();
            QStringList sl = avatar.split("?");
            if(sl.size() > 1) {
                sl.removeLast();
                avatar = sl.join("?");
            }
            sl = avatar.split(".");
            if(sl.size() < 2) {
                qWarning() << "AVATAR wrong "+speakerAPI->avatar();
            } else {
                // check if modified
                if(speaker->hasSpeakerImage()) {
                    if(speaker->speakerImageAsDataObject()->originImageUrl() != avatar) {
                        qDebug() << "IMAGE Changed";
                        SpeakerImage* speakerImage = speaker->speakerImageAsDataObject();
                        speakerImage->setOriginImageUrl(avatar);
                        speakerImage->setSuffix(sl.last());
                        mMultiSpeakerImages.insert(false, speakerImage);
                    }
                } else {
                    SpeakerImage* speakerImage = nullptr;
                    qDebug() << "IMAGE already in ASSETS ?";
                    speakerImage = mDataManager->findSpeakerImageBySpeakerId(speaker->speakerId());
                    if(!speakerImage) {
                        qDebug() << "IMAGE NEW";
                        speakerImage = mDataManager->createSpeakerImage();
                        speakerImage->setSpeakerId(speaker->speakerId());
                        speakerImage->setOriginImageUrl(avatar);
                        speakerImage->setSuffix(sl.last());
                        speakerImage->setInAssets(false);
                        speaker->resolveSpeakerImageAsDataObject(speakerImage);
                        mMultiSpeakerImages.insert(false, speakerImage);
                    } else {
                        qDebug() << "IMAGE found from ASSETS";
                        speaker->resolveSpeakerImageAsDataObject(speakerImage);
                    }
                }
            } // end if valid Avatar URL
        } // end check avatar if URL && not default
        else {
            qDebug() << "Speaker has NO Avatar";
        }
        // using MultiMap to get Speakers sorted
        mMultiSpeaker.insert(speaker->sortKey(), speaker);
    } // for speaker from server
    //
    qDebug() << "SPEAKERS: " << mDataManager->mAllSpeaker.size() << " --> " << mMultiSpeaker.size() << " IMG: " << mMultiSpeakerImages.size();
    mProgressInfotext.append("\n").append(tr("Sync Speaker Images"));
    emit progressInfo(mProgressInfotext);

    updateSpeakerImages();
}

void DataUtil::updateSpeakerImages() {
    if(!mMultiSpeakerImages.empty()) {
        QList<SpeakerImage*> waitingForDownload = mMultiSpeakerImages.values(false);
        if(!waitingForDownload.empty()) {
            mProgressInfotext.append(".");
            emit progressInfo(mProgressInfotext);
            // DO IT
            const QString speakerImagesPath = mConferenceDataPath + "speakerImages/";
            SpeakerImage* speakerImage = waitingForDownload.first();
            QString fileName;
            fileName = "speaker_";
            fileName.append(QString::number(speakerImage->speakerId()));
            fileName.append('.');
            fileName.append(speakerImage->suffix());
            mImageLoader = new ImageLoader(speakerImage->originImageUrl(), speakerImagesPath+fileName, this);
            bool res = connect(mImageLoader, SIGNAL(loaded(QObject*, int, int)), this,
                               SLOT(onSpeakerImageUpdateLoaded(QObject*, int, int)));
            if (!res) {
                Q_ASSERT(res);
            }
            res = connect(mImageLoader, SIGNAL(loadingFailed(QObject*, QString)), this,
                          SLOT(onSpeakerImageUpdateFailed(QObject*, QString)));
            if (!res) {
                Q_ASSERT(res);
            }
            mImageLoader->loadSpeaker(speakerImage);
            return;
        } // waiting for download
    } // new images map
    // all speaker images done
    qDebug() << "ALL SPEAKER IMAGES DONE";

    bool sessionOK = updateSessions(201801);
    if(!sessionOK) {
        return;
    }
    sessionOK = updateSessions(201802);
    if(!sessionOK) {
        return;
    }
    mProgressInfotext.append("\n").append(tr("Schedule and Speaker successfully synchronized :)"));
    emit progressInfo(mProgressInfotext);

    finishUpdate();
}

bool DataUtil::updateSessions(const int conferenceId) {
    QString city;
    if(conferenceId == 201801) {
        city = "BOSTON";
    } else {
        city = "BERLIN";
    }
    mProgressInfotext.append("\n").append(tr("Sync Sessions ")).append(city);

    Conference* conference;
    conference = static_cast<Conference*> (mDataManager->findConferenceById(conferenceId));
    if(!conference) {
        qWarning() << "No 'conference' found in prepared data for " << city;
        emit updateFailed(tr("Error: Data missed 'conference'.")+" "+city);
        return false;
    }

    // delete old generic sessions
    int firstGenericSession = conference->id()*100*-1;
    int lastGenericSession = conference->lastGenericSessionId();
    if(lastGenericSession == firstGenericSession) {
        qDebug() << "NO GENERIC SESSIONS to be deleted";
    } else {
        for (int i = lastGenericSession; i < firstGenericSession; ++i) {
            bool ok = mDataManager->deleteSessionBySessionId(i);
            if(!ok) {
                qDebug() << "cannot delete GENERIC SESSION " << i;
            } else {
                qDebug() << "GENERIC SESSION deleted " << i;
            }
        } // loop old generic sessions
        // reset counter
        conference->setLastGenericSessionId(conference->id()*100*-1);
    } // loop delete generic sessions

    emit progressInfo(mProgressInfotext);

    const QString schedulePath = mConferenceDataPath + "schedule_"+QString::number(conferenceId)+".json";
    QVariantMap map;
    map = readScheduleFile(schedulePath);
    if(map.isEmpty()) {
        qWarning() << "Schedule is no Map for " << city;
        emit updateFailed(tr("Error: Received Map is empty.")+" "+city);
        return false;
    }
    map = map.value("conference").toMap();
    if(map.isEmpty()) {
        qWarning() << "No 'conference' found in server response for " << city;
        emit updateFailed(tr("Error: Received Map missed 'conference'.")+" "+city);
        return false;
    }
    QVariantList serverDayList;
    QVariantMap allDaysMap;
    allDaysMap = map.value("days").toMap();
    QString myDay;
    if(conferenceId == 201801) {
        myDay= "2018-10-29";
        if(allDaysMap.contains(myDay)) {
            serverDayList.append(allDaysMap.value(myDay).toMap());
        } else {
            qDebug() << "Day missed in conference-days from server API " << myDay;
        }
        myDay = "2018-10-30";
        if(allDaysMap.contains(myDay)) {
            serverDayList.append(allDaysMap.value(myDay).toMap());
        } else {
            qDebug() << "Day missed in conference-days from server API " << myDay;
        }
    } else {
        myDay= "2018-12-05";
        if(allDaysMap.contains(myDay)) {
            serverDayList.append(allDaysMap.value(myDay).toMap());
        } else {
            qDebug() << "Day missed in conference-days from server API " << myDay;
        }
        myDay = "2018-12-06";
        if(allDaysMap.contains(myDay)) {
            serverDayList.append(allDaysMap.value(myDay).toMap());
        } else {
            qDebug() << "Day missed in conference-days from server API " << myDay;
        }
    }

    if(serverDayList.isEmpty()) {
        qWarning() << "No 'days' found for" << city;
        emit updateFailed(tr("Error: Received Map missed 'days'.")+" "+city);
        return false;
    }
    if(serverDayList.size() > conference->days().size()) {
        qWarning() << "too many 'days' found " << city;
        emit updateFailed(tr("Error: # of 'days' expected: ")+QString::number(conference->days().size())+" got: "+QString::number(serverDayList.size()));
        return false;
    }
    if(serverDayList.size() < conference->days().size()) {
        qWarning() << "some 'days' missing " << city << " found " << serverDayList.size() << " expected "<< conference->days().size();
    }
    for (int i = 0; i < serverDayList.size(); ++i) {
        QVariantMap dayMap;
        dayMap = serverDayList.at(i).toMap();
        if(dayMap.isEmpty()) {
            qWarning() << "No 'DAY' found #" << i << " " << city;
            // emit updateFailed(tr("Map for Day missed from Server")+" "+city);
            continue;
        }
        QString dayDate;
        dayDate = dayMap.value("date").toString();
        qDebug() << "processing DATE: " << dayDate;
        Day* day = findDayForServerDate(dayDate, conference);
        if(!day) {
            qWarning() << "No Day* found for " << dayDate << " " << city;
            // emit updateFailed(tr("No Day* found for ")+dayDate+" "+city);
            continue;
        }
        bool found = false;
        QVariantMap roomMap;
        roomMap = dayMap.value("rooms").toMap();
        // the keys are the room names
        QStringList roomKeys = roomMap.keys();
        if(roomKeys.isEmpty()) {
            qWarning() << "No 'ROOMS' found for DAY # i" << " " << city;
            // emit updateFailed(tr("No 'ROOMS' found for DAY ") + dayDate+" "+city);
            continue;
        }
        // loop thru room names
        qDebug() << "Searching Rooms from conference #" << conference->rooms().size() << " for server room names #" << roomKeys.size();
        for (int r = 0; r < roomKeys.size(); ++r) {
            QVariantList sessionList;
            sessionList = roomMap.value(roomKeys.at(r)).toList();
            if(sessionList.isEmpty()) {
                qWarning() << "DAY: " << dayDate << " ROOM: " << roomKeys.at(r) << " ignored - No Sessions available " << city;
                continue;
            }
            Room* room = nullptr;
            found = false;
            for (int rl = 0; rl < conference->rooms().size(); ++rl) {
                room = conference->rooms().at(rl);
                if(room->roomName() == roomKeys.at(r)) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                qDebug() << "Room* not found for " << dayDate << " Room: " << roomKeys.at(r);
                if(roomKeys.at(r).isEmpty()) {
                    // use dummi room
                    room = static_cast<Room*>( mDataManager->allRoom().first());
                    qDebug() << "Room Name empty - using Room " << room->roomName() << "for " << city;
                } else {
                    room = mDataManager->createRoom();
                    conference->setLastRoomId(conference->lastRoomId()+1);
                    room->setRoomId(conference->lastRoomId());
                    room->setInAssets(false);
                    room->setRoomName(roomKeys.at(r));
                    mDataManager->insertRoom(room);
                    conference->addToRooms(room);
                    mProgressInfotext.append("R");
                    progressInfo(mProgressInfotext);
                }
            }
            for (int sl = 0; sl < sessionList.size(); ++sl) {
                QVariantMap sessionMap;
                sessionMap = sessionList.at(sl).toMap();
                if(sessionMap.isEmpty()) {
                    qWarning() << "No 'SESSION' Map DAY: " << dayDate << " ROOM: " << roomKeys.at(r) << " for " << city;
                    continue;
                }
                // adjust persons
                adjustPersons(sessionMap);
                // adjust tracks (true: isUpdate == not in assets
                adjustTracks(sessionMap, conference, true);
                // ekke TODO fix generator bug (without resolving we get tracks doubled)
                QList<SessionTrack*> tList;
                conference->resolveTracksKeys(tList);

                SessionAPI* sessionAPI = mDataManager->createSessionAPI();
                sessionAPI->fillFromForeignMap(sessionMap);
                // ignore unwanted Sessions
                if (checkIfIgnored(sessionAPI)) {
                    continue;
                }

                Session* session = mDataManager->findSessionBySessionId(sessionAPI->sessionId());
                if(!session) {
                    // NEW
                    qDebug() << "NEW Session " << QString::number(sessionAPI->sessionId());
                    mProgressInfotext.append("+");
                    session = mDataManager->createSession();
                    session->setSessionId(sessionAPI->sessionId());
                } else {
                    // Update
                    qDebug() << "Update Session " << QString::number(sessionAPI->sessionId());
                    mProgressInfotext.append(".");
                }
                emit progressInfo(mProgressInfotext);
                session->fillFromMap(sessionAPI->toMap());
                setDuration(sessionAPI, session);
                // refs
                // DAY
                session->setSessionDay(day->id());
                session->resolveSessionDayAsDataObject(day);
                // ROOM
                session->setRoom(room->roomId());
                session->resolveRoomAsDataObject(room);
                // TYPE SCHEDULE
                setType(session);
                // SessionLinks

                // SORT
                session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
                mMultiSession.insert(session->sortKey(), session);
            } // end for sessions of a room of a day
        } // end for rooms of a day
    } // end for list of days from server

    qDebug() << "SESSIONS: " << mDataManager->mAllSession.size() << " --> " << mMultiSession.size();
    qDebug() << "Conference " << conference->conferenceCity() << " with days #" << conference->days().size() << " rooms #" << conference->rooms().size();
    mProgressInfotext.append("\n").append(tr("Schedule successfully synchronized :)")).append(" ").append(city);
    emit progressInfo(mProgressInfotext);
    return true;
}

void DataUtil::addGenericSessionsBoston201801() {
    int conferenceId = 201801;
    Conference* conference;
    conference = static_cast<Conference*> (mDataManager->findConferenceById(conferenceId));
    if(!conference) {
        qWarning() << "No 'conference' found - cannot add Generic Sessions ";
        emit updateFailed(tr("Error: Data missed 'conference'.")+QString::number(conferenceId));
        return;
    }
    int lastGenericSession = conference->lastGenericSessionId();
    Session* session = nullptr;
    for (int i = 0; i < conference->days().size(); ++i) {
        Day* day = conference->days().at(i);
        if(day->conferenceDay().toString(YYYY_MM_DD) == "2018-10-29") {
            // Tech Day
            // REGISTRATION
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Registration and Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsRegistration(true);
            session->setStartTime(QTime::fromString("08:00", HH_MM));
            session->setEndTime(QTime::fromString("09:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // LUNCH
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Lunch"));
            session->setIsGenericScheduleSession(true);
            session->setIsLunch(true);
            session->setStartTime(QTime::fromString("12:00", HH_MM));
            session->setEndTime(QTime::fromString("13:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Break"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("15:00", HH_MM));
            session->setEndTime(QTime::fromString("15:30", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // NETWORKING
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Networking and Drinks"));
            session->setIsGenericScheduleSession(true);
            session->setIsEvent(true);
            session->setStartTime(QTime::fromString("17:15", HH_MM));
            session->setEndTime(QTime::fromString("19:00", HH_MM));
            session->setMinutes(105);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
        } else {
            // CONF DAY
            // REGISTRATION
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Registration and Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsRegistration(true);
            session->setStartTime(QTime::fromString("08:00", HH_MM));
            session->setEndTime(QTime::fromString("09:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK COFFEE
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("10:30", HH_MM));
            session->setEndTime(QTime::fromString("11:00", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // LUNCH
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Lunch"));
            session->setIsGenericScheduleSession(true);
            session->setIsLunch(true);
            session->setStartTime(QTime::fromString("12:00", HH_MM));
            session->setEndTime(QTime::fromString("13:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Break"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("14:30", HH_MM));
            session->setEndTime(QTime::fromString("15:00", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // NETWORKING
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Networking and Drinks"));
            session->setIsGenericScheduleSession(true);
            session->setIsEvent(true);
            session->setStartTime(QTime::fromString("17:15", HH_MM));
            session->setEndTime(QTime::fromString("19:00", HH_MM));
            session->setMinutes(105);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
        }
    }
    conference->setLastGenericSessionId(lastGenericSession);
}

void DataUtil::addGenericSessionsBerlin201802() {
    int conferenceId = 201802;
    Conference* conference;
    conference = static_cast<Conference*> (mDataManager->findConferenceById(conferenceId));
    if(!conference) {
        qWarning() << "No 'conference' found - cannot add Generic Sessions ";
        emit updateFailed(tr("Error: Data missed 'conference'.")+QString::number(conferenceId));
        return;
    }
    int lastGenericSession = conference->lastGenericSessionId();
    Session* session = nullptr;
    for (int i = 0; i < conference->days().size(); ++i) {
        Day* day = conference->days().at(i);
        if(day->conferenceDay().toString(YYYY_MM_DD) == "2018-12-05") {
            // Tech Day
            // REGISTRATION
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Registration and Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsRegistration(true);
            session->setStartTime(QTime::fromString("08:00", HH_MM));
            session->setEndTime(QTime::fromString("09:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // LUNCH
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Lunch"));
            session->setIsGenericScheduleSession(true);
            session->setIsLunch(true);
            session->setStartTime(QTime::fromString("12:00", HH_MM));
            session->setEndTime(QTime::fromString("13:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Break"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("15:00", HH_MM));
            session->setEndTime(QTime::fromString("15:30", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // NETWORKING
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Networking and Drinks"));
            session->setIsGenericScheduleSession(true);
            session->setIsEvent(true);
            session->setStartTime(QTime::fromString("17:15", HH_MM));
            session->setEndTime(QTime::fromString("19:00", HH_MM));
            session->setMinutes(105);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
        } else {
            // CONF DAY
            // REGISTRATION
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Registration and Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsRegistration(true);
            session->setStartTime(QTime::fromString("08:00", HH_MM));
            session->setEndTime(QTime::fromString("09:00", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK COFFEE
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Coffee"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("10:30", HH_MM));
            session->setEndTime(QTime::fromString("11:00", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // LUNCH
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Lunch"));
            session->setIsGenericScheduleSession(true);
            session->setIsLunch(true);
            session->setStartTime(QTime::fromString("12:30", HH_MM));
            session->setEndTime(QTime::fromString("13:30", HH_MM));
            session->setMinutes(60);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // BREAK
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Break"));
            session->setIsGenericScheduleSession(true);
            session->setIsBreak(true);
            session->setStartTime(QTime::fromString("15:00", HH_MM));
            session->setEndTime(QTime::fromString("15:45", HH_MM));
            session->setMinutes(30);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
            // NETWORKING
            session = mDataManager->createSession();
            lastGenericSession --;
            session->setSessionId(lastGenericSession);
            session->setTitle(tr("Networking and Drinks"));
            session->setIsGenericScheduleSession(true);
            session->setIsEvent(true);
            session->setStartTime(QTime::fromString("17:15", HH_MM));
            session->setEndTime(QTime::fromString("19:00", HH_MM));
            session->setMinutes(105);
            session->setConference(conferenceId);
            session->setSessionDay(day->id());
            session->setSortKey(day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString(HH_MM));
            mMultiSession.insert(session->sortKey(), session);
            mDataManager->insertSession(session);
        }
    }
    conference->setLastGenericSessionId(lastGenericSession);
}

void DataUtil::finishUpdate() {
    mProgressInfotext.append("\n").append(tr("Now saving Conference Data to Cache"));
    emit progressInfo(mProgressInfotext);

    // Room: clear sessions for update
    for (int r = 0; r < mDataManager->allRoom().size(); ++r) {
        Room* room = static_cast<Room*>( mDataManager->allRoom().at(r));
        room->clearSessions();
    }
    // to reset all correct
    resolveSessionsForRooms();
    qDebug() << "FINISH: Rooms sessions cleared";
    // SessionTrack: clear sessions for update
    for (int st = 0; st < mDataManager->allSessionTrack().size(); ++st) {
        SessionTrack* track = static_cast<SessionTrack*>( mDataManager->allSessionTrack().at(st));
        track->clearSessions();
        qDebug() << "clear sessions for Track " << track->name();
    }
    // to reset all correct
    resolveSessionsForTracks();
    qDebug() << "FINISH: Tracks sessions cleared";
    // Day: clear sessions for update
    for (int d = 0; d < mDataManager->allDay().size(); ++d) {
        Day* day = static_cast<Day*>( mDataManager->allDay().at(d));
        day->clearSessions();
    }
    // to reset all correct
    resolveSessionsForSchedule();
    qDebug() << "FINISH: Days sessions cleared";
    // Speaker: insert sorted Speakers, clear Sessions
    mDataManager->mAllSpeaker.clear();
    QMapIterator<QString, Speaker*> speakerIterator(mMultiSpeaker);
    while (speakerIterator.hasNext()) {
        speakerIterator.next();
        Speaker* speaker = speakerIterator.value();
        speaker->clearSessions();
        resolveSessionsForSpeaker(speaker);
        mDataManager->insertSpeaker(speaker);
    }
    qDebug() << "FINISH: Sorted Speakers inserted";

    // Check orphans
    for (int i = 0; i < mDataManager->mAllSession.size(); ++i) {
        Session* session = static_cast<Session*>( mDataManager->mAllSession.at(i));
        bool sessionFound = false;
        QMapIterator<QString, Session*> sessionIterator(mMultiSession);
        while (sessionIterator.hasNext()) {
            sessionIterator.next();
            Session* updatedSession = sessionIterator.value();
            if(updatedSession->sessionId() == session->sessionId()) {
                sessionFound = true;
                break;
            }
        }
        if(!sessionFound) {
            session->deleteLater();
            qDebug() << "Session removed: " << session->sessionId();
        }
    }
    qDebug() << "FINISH: Session orphans deleted";

    // Session: insert sorted Sessions
    // presenter, sessionLinks, day, room, track scheduleItem are updated
    mDataManager->mAllSession.clear();
    // now add Generic Sessions
    addGenericSessionsBoston201801();
    addGenericSessionsBerlin201802();
    qDebug() << "FINISH: ScheduleItems added";
    // now insert sorted Sessions, update sessions for Day, Room, Tracks, Speaker
    sortedSessionsIntoRoomDayTrackSpeaker();
    qDebug() << "FINISH: Rooms Days Tracks Speaker Sessions sorted";

    // SPEAKER
    mDataManager->saveSpeakerToCache();
    qDebug() << "FINISH: Speaker saved";

    // insert Speaker Images
    // do NOT mDataManager->mAllSpeakerImage.clear();
    // mMultiSpeakerImages contains NEW or UPDATED Speaker Images !
    QMapIterator<bool, SpeakerImage*> speakerImagesIterator(mMultiSpeakerImages);
    while (speakerImagesIterator.hasNext()) {
        speakerImagesIterator.next();
        SpeakerImage* speakerImage = speakerImagesIterator.value();
        if(!mDataManager->findSpeakerImageBySpeakerId(speakerImage->speakerId())) {
            // a NEW Speaker Image
            qDebug() << "detcted NEW Speaker Image: " << speakerImage->speakerId();
            mDataManager->insertSpeakerImage(speakerImage);
        } else {
            qDebug() << "detcted UPDATED Speaker Image: " << speakerImage->speakerId();
        }
    }
    mDataManager->saveSpeakerImageToCache();
    qDebug() << "FINISH: Speaker Images saved";

    // Track sort by Name
    QMultiMap<QString, SessionTrack*> sessionTrackSortMap;
    for (int i = 0; i < mDataManager->allSessionTrack().size(); ++i) {
        SessionTrack* sessionTrack = static_cast<SessionTrack*>( mDataManager->allSessionTrack().at(i));
        sessionTrackSortMap.insert(sessionTrack->name(), sessionTrack);
    }
    qDebug() << "FINISH: Tracks sorted by Name";
    mDataManager->mAllSessionTrack.clear();
    QMapIterator<QString, SessionTrack*> sessionTrackIterator(sessionTrackSortMap);
    while (sessionTrackIterator.hasNext()) {
        sessionTrackIterator.next();
        SessionTrack* sessionTrack = sessionTrackIterator.value();
        mDataManager->insertSessionTrack(sessionTrack);
    }
    // save all Tracks with sorted Sessions
    mDataManager->saveSessionTrackToCache();
    qDebug() << "FINISH: Tracks saved";

    // Day: save all days with sorted Sessions
    mDataManager->saveDayToCache();
    qDebug() << "FINISH: Days saved";

    // Rooms: save all Rooms with sorted Sessions
    mDataManager->saveRoomToCache();
    qDebug() << "FINISH: Rooms saved";

    // Sessions
    mDataManager->saveSessionToCache();
    qDebug() << "FINISH: Sessions saved";

    // SETTINGS update API
    mDataManager->mSettingsData->setApiVersion(mNewApi);
    mDataManager->mSettingsData->setLastUpdate(QDateTime::currentDateTime());
    mDataManager->mSettingsData->setVersion(2018006);
    mDataManager->saveSettings();

    // SAVE CONFERENCES
    mDataManager->saveConferenceToCache();
    qDebug() << "FINISH: Conference saved";

    mCurrentConference = nullptr;
    mCurrentConference = currentConference();

    //
    mProgressInfotext.append("\n").append(tr("All done"));
    emit progressInfo(mProgressInfotext);
    qDebug() << "FINISH: All Done";
    emit updateDone();
}

//  U T I L I T Y S  to manage Conference data

/**
 * Favorites are transient on Sessions
 * Sessions are read-only
 * So at startup the propertie will be set
 */
void DataUtil::setSessionFavorites()
{
    for (int i = 0; i < mDataManager->allFavorite().size(); ++i) {
        Favorite* favorite = static_cast<Favorite*>( mDataManager->allFavorite().at(i));
        Session* session = mDataManager->findSessionBySessionId(favorite->sessionId());
        if(session != nullptr) {
            session->setIsFavorite(true);
        }
    }
}

/**
 * Favorites are transient on Sessions
 * Sessions are read-only
 * So while caching data Favorites will be created
 */
void DataUtil::saveSessionFavorites()
{
    mDataManager->mAllFavorite.clear();
    for (int i = 0; i < mDataManager->mAllSession.size(); ++i) {
        Session* session = static_cast<Session*>( mDataManager->mAllSession.at(i));
        if(session->isFavorite()) {
            Favorite* favorite = mDataManager->createFavorite();
            favorite->setSessionId(session->sessionId());
            mDataManager->insertFavorite(favorite);
        }
    }
}

/**
 * list of days for a conference and list of sessions for a Day are lazy
 * (only IDs stored in an Array)
 * for the Conference we always need all to create the schedule
 * so at startup or update this will be called
 */
void DataUtil::resolveSessionsForSchedule() {
    for (int i = 0; i < mDataManager->allConference().size(); ++i) {
        Conference* conference = static_cast<Conference*>( mDataManager->allConference().at(i));
        conference->resolveDaysKeys(mDataManager->listOfDayForKeys(conference->daysKeys()));
        for (int d = 0; d < conference->days().size(); ++d) {
            Day* day = conference->days().at(d);
            // cannot use my  generated method, because it's loosing the sort
            // TODO GENERATOR bug fix
            // day->resolveSessionsKeys(mDataManager->listOfSessionForKeys(day->sessionsKeys()));
            day->resolveSessionsKeys(listOfSessionForSortedKeys(day->sessionsKeys()));
        }
        conference->resolveRoomsKeys(mDataManager->listOfRoomForKeys(conference->roomsKeys()));
        conference->resolveTracksKeys(mDataManager->listOfSessionTrackForKeys(conference->tracksKeys()));
    }
}

// TODO remove if Generator is fixed
QList<Session*> DataUtil::listOfSessionForSortedKeys(
        QStringList keyList)
{
    QList<Session*> listOfData;
    keyList.removeDuplicates();
    for (int k = 0; k < keyList.size(); ++k) {
        int theSessionId = keyList.at(k).toInt();
        for (int i = 0; i < mDataManager->allSession().size(); ++i) {
            Session* session;
            session = static_cast<Session*>( mDataManager->allSession().at(i));
            if(session->sessionId() == theSessionId) {
                listOfData.append(session);
                break;
            }
        } // loop sessions
    } // loop keys
    if(listOfData.size() != keyList.size()) {
        qDebug() << "not all found for keys";
    }
    return listOfData;
}

/**
 * list of sessions for a Track are lazy
 * (only IDs stored in an Array)
 * this will be called as soon as Tracks are first time displayed
 */
void DataUtil::resolveSessionsForTracks()
{
    for (int i = 0; i < mDataManager->mAllSessionTrack.size(); ++i) {
        SessionTrack* sessionTrack = static_cast<SessionTrack*>( mDataManager->mAllSessionTrack.at(i));
        sessionTrack->resolveSessionsKeys(mDataManager->listOfSessionForKeys(sessionTrack->sessionsKeys()));
    }
}

void DataUtil::resolveSessionsForRooms()
{
    for (int i = 0; i < mDataManager->mAllRoom.size(); ++i) {
        Room* room = static_cast<Room*>( mDataManager->mAllRoom.at(i));
        room->resolveSessionsKeys(mDataManager->listOfSessionForKeys(room->sessionsKeys()));
    }
}

void DataUtil::resolveSessionsForSpeaker(Speaker* speaker)
{
    speaker->resolveSessionsKeys(mDataManager->listOfSessionForKeys(speaker->sessionsKeys()));
}

Conference* DataUtil::switchConference() {
    if(!mCurrentConference) {
        return currentConference();
    }
    if(mCurrentConference == static_cast<Conference*>( mDataManager->allConference().first())) {
        mCurrentConference = static_cast<Conference*>( mDataManager->allConference().last());
    } else {
        mCurrentConference = static_cast<Conference*>( mDataManager->allConference().first());
    }
    return mCurrentConference;
}

QString DataUtil::conferenceCity(const int conferenceId)
{
    if(mCurrentConference) {
        if(mCurrentConference->id() == conferenceId) {
            return mCurrentConference->conferenceCity();
        }
    }
    Conference* conference = mDataManager->findConferenceById(conferenceId);
    if(conference) {
        return conference->conferenceCity();
    }
    return "";
}

QString DataUtil::otherConferenceCity()
{
    if(!mCurrentConference) {
        currentConference();
    }
    if(mCurrentConference) {
        Conference* conference = nullptr;
        if(mCurrentConference->id() == 201801) {
            conference = mDataManager->findConferenceById(201802);
        } else {
            conference = mDataManager->findConferenceById(201801);
        }
        if(conference) {
            return conference->conferenceCity();
        }
    }
    return "";
}

Conference* DataUtil::currentConference() {
    if(mDataManager->allConference().empty()) {
        qDebug() << "cpp currentConference() --> fresh start - no conferences yet";
        return mCurrentConference;
    }
    if(!mCurrentConference) {
        // TODO depends from current date
        // if currentDate > last day of first conference: use the second one
        mCurrentConference = static_cast<Conference*>( mDataManager->allConference().last());
        qDebug() << "Current Conference is last: " << mCurrentConference->conferenceCity();
    }
    return mCurrentConference;
}

QString DataUtil::scheduleTabName(int tabBarIndex)
{
    if(!mCurrentConference) {
        currentConference();
    }
    if(mCurrentConference->days().size()<(tabBarIndex +1)) {
        qWarning() << "Curren Conference has less Days ";
        return "??";
    }
    Day* day = mCurrentConference->days().at(tabBarIndex);
    //return day->conferenceDay().toString("ddd (dd)");
    if(mCurrentConference->days().size() > 3) {
        return QDate::shortDayName(day->conferenceDay().dayOfWeek());
    }
    return QDate::longDayName(day->conferenceDay().dayOfWeek());
}

SessionLists *DataUtil::mySchedule()
{
    return mSessionLists;
}

void DataUtil::refreshMySchedule()
{
    if(!mCurrentConference) {
        currentConference();
    }
    mSessionLists->clearScheduledSessions();
    for (int d = 0; d < mCurrentConference->days().size(); ++d) {
        Day* day = mCurrentConference->days().at(d);
        for (int s = 0; s < day->sessions().size(); ++s) {
            Session* session = static_cast<Session*> (day->sessions().at(s));
            if(!session->isDeprecated() && session->isFavorite()) {
                mSessionLists->addToScheduledSessions(session);
            }
        }
    }
    qDebug() << "MY SCHEDLUE #:" << mSessionLists->scheduledSessionsCount();
    emit myScheduleRefreshed();
}

int DataUtil::findFirstSessionItem(int conferenceDayIndex, QString pickedTime)
{
    if(!mCurrentConference) {
        currentConference();
    }
    if(conferenceDayIndex < 0 || conferenceDayIndex > (mCurrentConference->days().size()-1)) {
        qDebug() << "Day Index wrong: conferenceDayIndex";
        return -1;
    }
    Day* day = static_cast<Day*> (mCurrentConference->days().at(conferenceDayIndex));
    for (int i = 0; i < day->sessions().size(); ++i) {
        Session* session = day->sessions().at(i);
        QString theTime = session->sortKey().right(5);
        if(theTime >= pickedTime) {
            return i;
        }
    }
    return day->sessions().size();
}

int DataUtil::findFirstSpeakerItem(QString letter)
{
    for (int i = 0; i < mDataManager->mAllSpeaker.size(); ++i) {
        Speaker* speaker = static_cast<Speaker*>( mDataManager->mAllSpeaker.at(i));
        if(speaker->sortGroup() >= letter) {
            return i;
        }
    }
    return mDataManager->mAllSpeaker.size();
}

// Sortkey: day->conferenceDay().toString(YYYY_MM_DD)+session->startTime().toString("HH:mm")
QString DataUtil::localWeekdayAndTime(QString sessionSortkey)
{
    QDateTime sortTime = QDateTime::fromString(sessionSortkey, YYYY_MM_DD_HH_MM);
    QString weekDayAndTime = sortTime.toString(DAYNAME);
    weekDayAndTime.append(", ");
    QTime myTime = QTime::fromString(sessionSortkey.right(5));
    weekDayAndTime.append(myTime.toString(Qt::SystemLocaleShortDate));
    return weekDayAndTime;
}

QString DataUtil::apiInfo()
{
    QString apiInfo = tr("API Version ");
    apiInfo.append(mDataManager->settingsData()->apiVersion());
    apiInfo.append(("\n")).append(tr("Last Update "));
    if(mDataManager->mSettingsData->hasLastUpdate()) {
        apiInfo.append(mDataManager->mSettingsData->lastUpdate().toString(Qt::SystemLocaleShortDate));
    } else {
        apiInfo.append("n/a");
    }
    return apiInfo;
}


//   DOWNLOAD   S P E A K E R I M A G E S
// SLOT   UPDATE CONFERENCE
void DataUtil::onSpeakerImageUpdateLoaded(QObject *dataObject, int width, int height)
{
    mImageLoader->deleteLater();
    SpeakerImage* speakerImage = static_cast<SpeakerImage*>( dataObject);
    qDebug() << "onSpeakerImage  L O A D E D ";
    speakerImage->setDownloadSuccess(true);
    speakerImage->setDownloadFailed(false);
    speakerImage->setInAssets(false);
    speakerImage->setInData(true);
    prepareHighDpiImages(speakerImage, width, height);
    // set update flag
    int count = mMultiSpeakerImages.remove(false, speakerImage);
    if(count != 1) {
        qWarning() << "something went wrong: the SpeakerImage MUST exist in MultiMap";
    }
    mMultiSpeakerImages.insert(true, speakerImage);
    // check for more
    updateSpeakerImages();
}
void DataUtil::onSpeakerImageUpdateFailed(QObject *dataObject, QString message) {
    mImageLoader->deleteLater();
    SpeakerImage* speakerImage = static_cast<SpeakerImage*>( dataObject);
    qDebug() << "UPDATE: Cannot load Speaker Image:  " << message << speakerImage->speakerId();
    speakerImage->setDownloadSuccess(false);
    speakerImage->setDownloadFailed(true);
    speakerImage->setInAssets(false);
    speakerImage->setInData(false);
    // set update flag
    int count = mMultiSpeakerImages.remove(false, speakerImage);
    if(count != 1) {
        qWarning() << "something went wrong: the SpeakerImage MUST exist in MultiMap";
    }
    mMultiSpeakerImages.insert(true, speakerImage);
    // check for more
    updateSpeakerImages();
}

void DataUtil::prepareHighDpiImages(SpeakerImage* speakerImage, int width, int height) {
    const QString speakerImagesPath = mConferenceDataPath + "speakerImages/";
    QString fileName;
    fileName = speakerImagesPath + "speaker_";
    fileName.append(QString::number(speakerImage->speakerId()));
    QString originFileName;
    originFileName = fileName + "." + speakerImage->suffix();
    const int size1 = 96;
    const int size2 = 192;
    const int size3 = 288;
    const int size4 = 384;
    if(width >= height) {
        if(width < size1) {
            speakerImage->setMaxScaleFactor(0);
            return;
        }
        QFile originFile(originFileName);
        if(!originFile.exists()) {
            qWarning() << "SpeakerImage Path not found " << originFileName;
            return;
        }
        if (!originFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open file: " << originFileName;
            return;
        }
        QImage originImage = QImage::fromData(originFile.readAll());
        if(originImage.isNull()) {
            qWarning() << "Cannot construct Image from file: " << originFileName;
            return;
        }
        originImage.save(fileName+"_origin."+speakerImage->suffix());
        QImage scaledImage;
        if(width >= size1) {
            scaledImage = originImage.scaledToWidth(size1);
            scaledImage.save(originFileName);
            speakerImage->setMaxScaleFactor(1);
        }
        if(width >= size2) {
            scaledImage = originImage.scaledToWidth(size2);
            scaledImage.save(fileName+"@2x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(2);
        }
        if(width >= size3) {
            scaledImage = originImage.scaledToWidth(size3);
            scaledImage.save(fileName+"@3x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(3);
        }
        if(width >= size4) {
            scaledImage = originImage.scaledToWidth(size4);
            scaledImage.save(fileName+"@4x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(4);
        }
    } else {
        if(height < size1) {
            speakerImage->setMaxScaleFactor(0);
            return;
        }
        QFile originFile(originFileName);
        if(!originFile.exists()) {
            qWarning() << "SpeakerImage Path not found " << originFileName;
            return;
        }
        if (!originFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open file: " << originFileName;
            return;
        }
        QImage originImage = QImage::fromData(originFile.readAll());
        if(originImage.isNull()) {
            qWarning() << "Cannot construct Image from file: " << originFileName;
            return;
        }
        originImage.save(fileName+"_origin."+speakerImage->suffix());
        QImage scaledImage;
        if(height >= size1) {
            scaledImage = originImage.scaledToHeight(size1);
            scaledImage.save(originFileName);
            speakerImage->setMaxScaleFactor(1);
        }
        if(height >= size2) {
            scaledImage = originImage.scaledToHeight(size2);
            scaledImage.save(fileName+"@2x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(2);
        }
        if(height >= size3) {
            scaledImage = originImage.scaledToHeight(size3);
            scaledImage.save(fileName+"@3x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(3);
        }
        if(height >= size4) {
            scaledImage = originImage.scaledToHeight(size4);
            scaledImage.save(fileName+"@4x."+speakerImage->suffix());
            speakerImage->setMaxScaleFactor(4);
        }
    }
}

// S L O T S
void DataUtil::onServerSuccess()
{
    qDebug() << "S U C C E S S request Schedule (BOSTON, BERLIN) and Speaker";

    // check if conference is prepared
    if(isOldConference() || mDataManager->allConference().empty() || mDataManager->settingsData()->version() < 2018005) {
        prepareConference();
    }
    continueUpdate();
}

void DataUtil::onVersionSuccess(QByteArray currentVersionBytes)
{
    QJsonDocument jda;
    jda = QJsonDocument::fromJson(currentVersionBytes);
    if(!jda.isObject()) {
        onVersionFailed(tr("No JSON Object received for Version Check"));
        return;
    }
    QVariantMap versionMap = jda.toVariant().toMap();
    if(versionMap.isEmpty()) {
        onVersionFailed(tr("Version Check: received Version Map is empty"));
        return;
    }
    if(!versionMap.contains("version")) {
        onVersionFailed(tr("Version Check: received Version Map missed Version Property"));
        return;
    }
    mNewApi = versionMap.value("version").toString();
    if(mNewApi.isEmpty()) {
        onVersionFailed(tr("Error: Received Map missed 'version'."));
        return;
    }
    qDebug() << "VERSION: " + mNewApi;

    QStringList versionList;
    versionList = mNewApi.split(".");
    if(versionList.size() != 2) {
        emit onVersionFailed(tr("Error: 'Version' wrong: ")+mNewApi);
        return;
    }

    if(mDataManager->mSettingsData->apiVersion().length() == 0) {
        emit updateAvailable(mNewApi);
        return;
    }

    if(mDataManager->settingsData()->version() < 2018006) {
        emit updateAvailable(mNewApi);
        return;
    }

    QStringList oldVersionList;
    oldVersionList = mDataManager->mSettingsData->apiVersion().split(".");
    if(oldVersionList.size() != 2) {
        emit updateAvailable(mNewApi);
        return;
    }

    int oldValue = oldVersionList.at(0).toInt();
    int newValue = versionList.at(0).toInt();
    if(oldValue > newValue) {
        emit noUpdateRequired();
        return;
    }
    oldValue = oldVersionList.at(0).toInt();
    newValue = versionList.at(0).toInt();
    if(oldValue < newValue) {
        emit updateAvailable(mNewApi);
        return;
    }
    oldValue = oldVersionList.at(1).toInt();
    newValue = versionList.at(1).toInt();
    if(oldValue <  newValue) {
        emit updateAvailable(mNewApi);
        return;
    }
    emit noUpdateRequired();
}

void DataUtil::onVersionFailed(const QString message)
{
    qDebug() << "Version failed" << message;
    emit checkForUpdateFailed(message);
}

void DataUtil::onServerFailed(QString message)
{
    qDebug() << "FAILED: " << message;
    emit updateFailed(message);
}

