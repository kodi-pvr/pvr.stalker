/*
 *      Copyright (C) 2015, 2016  Jamal Edey
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include "client.h"

#include <xbmc_pvr_dll.h>
#include <p8-platform/util/util.h>

#include "SData.h"

#define PORTAL_SUFFIX_FORMAT "%s_%d"

#define GET_SETTING_STR(name, tmp, store, def) \
  if (!XBMC->GetSetting(name, tmp)) \
    store = def; \
  else \
    store = tmp;

#define GET_SETTING_INT(name, store, def) \
  if (!XBMC->GetSetting(name, &store)) \
    store = def;

#define GET_SETTING_STR2(setting, name, portal, tmp, store, def) \
  sprintf(setting, PORTAL_SUFFIX_FORMAT, name, portal); \
  GET_SETTING_STR(setting, tmp, store, def);

#define GET_SETTING_INT2(setting, name, portal, store, def) \
  sprintf(setting, PORTAL_SUFFIX_FORMAT, name, portal); \
  GET_SETTING_INT(setting, store, def);

using namespace ADDON;

CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr *PVR = NULL;

std::string g_strUserPath = "";
std::string g_strClientPath = "";

SData *m_data = NULL;
ADDON_STATUS m_currentStatus = ADDON_STATUS_UNKNOWN;

extern "C" {

ADDON_STATUS ADDON_Create(void *callbacks, void *props) {
    if (!callbacks || !props)
        return ADDON_STATUS_UNKNOWN;

    XBMC = new CHelper_libXBMC_addon;
    if (!XBMC->RegisterMe(callbacks)) {
        SAFE_DELETE(XBMC);
        return ADDON_STATUS_PERMANENT_FAILURE;
    }

    PVR = new CHelper_libXBMC_pvr;
    if (!PVR->RegisterMe(callbacks)) {
        SAFE_DELETE(PVR);
        SAFE_DELETE(XBMC);
        return ADDON_STATUS_PERMANENT_FAILURE;
    }

    XBMC->Log(LOG_DEBUG, "%s: Creating the Stalker Client PVR Add-on", __FUNCTION__);

    PVR_PROPERTIES *pvrProps = (PVR_PROPERTIES *) props;
    g_strUserPath = pvrProps->strUserPath;
    g_strClientPath = pvrProps->strClientPath;

    m_data = new SData;

    if (!XBMC->DirectoryExists(g_strUserPath.c_str())) {
#ifdef TARGET_WINDOWS
        CreateDirectory(g_strUserPath.c_str(), NULL);
#else
        XBMC->CreateDirectory(g_strUserPath.c_str());
#endif
    }

    int portal;
    char buffer[1024];
    char setting[256];
    int enumTemp;

    GET_SETTING_INT("active_portal", m_data->settings.activePortal, SC_SETTINGS_DEFAULT_ACTIVE_PORTAL);
    GET_SETTING_INT("connection_timeout", m_data->settings.connectionTimeout, SC_SETTINGS_DEFAULT_CONNECTION_TIMEOUT);
    // calc based on index (5 second steps)
    m_data->settings.connectionTimeout *= 5;

    portal = m_data->settings.activePortal;
    GET_SETTING_STR2(setting, "mac", portal, buffer, m_data->settings.mac, SC_SETTINGS_DEFAULT_MAC);
    GET_SETTING_STR2(setting, "server", portal, buffer, m_data->settings.server, SC_SETTINGS_DEFAULT_SERVER);
    GET_SETTING_STR2(setting, "time_zone", portal, buffer, m_data->settings.timeZone, SC_SETTINGS_DEFAULT_TIME_ZONE);
    GET_SETTING_STR2(setting, "login", portal, buffer, m_data->settings.login, SC_SETTINGS_DEFAULT_LOGIN);
    GET_SETTING_STR2(setting, "password", portal, buffer, m_data->settings.password, SC_SETTINGS_DEFAULT_PASSWORD);
    GET_SETTING_INT2(setting, "guide_preference", portal, enumTemp, SC_SETTINGS_DEFAULT_GUIDE_PREFERENCE);
    m_data->settings.guidePreference = (SC::Settings::GuidePreference) enumTemp;
    GET_SETTING_INT2(setting, "guide_cache", portal, m_data->settings.guideCache, SC_SETTINGS_DEFAULT_GUIDE_CACHE);
    GET_SETTING_INT2(setting, "guide_cache_hours", portal, m_data->settings.guideCacheHours,
                     SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS);
    GET_SETTING_INT2(setting, "xmltv_scope", portal, enumTemp, SC_SETTINGS_DEFAULT_XMLTV_SCOPE);
    m_data->settings.xmltvScope = (HTTPSocket::Scope) enumTemp;
    if (m_data->settings.xmltvScope == HTTPSocket::Scope::SCOPE_REMOTE) {
        GET_SETTING_STR2(setting, "xmltv_url", portal, buffer, m_data->settings.xmltvPath,
                         SC_SETTINGS_DEFAULT_XMLTV_URL);
    } else {
        GET_SETTING_STR2(setting, "xmltv_path", portal, buffer, m_data->settings.xmltvPath,
                         SC_SETTINGS_DEFAULT_XMLTV_PATH);
    }
    GET_SETTING_STR2(setting, "token", portal, buffer, m_data->settings.token, SC_SETTINGS_DEFAULT_TOKEN);
    GET_SETTING_STR2(setting, "serial_number", portal, buffer, m_data->settings.serialNumber,
                     SC_SETTINGS_DEFAULT_SERIAL_NUMBER);
    GET_SETTING_STR2(setting, "device_id", portal, buffer, m_data->settings.deviceId, SC_SETTINGS_DEFAULT_DEVICE_ID);
    GET_SETTING_STR2(setting, "device_id2", portal, buffer, m_data->settings.deviceId2, SC_SETTINGS_DEFAULT_DEVICE_ID2);
    GET_SETTING_STR2(setting, "signature", portal, buffer, m_data->settings.signature, SC_SETTINGS_DEFAULT_SIGNATURE);

    XBMC->Log(LOG_DEBUG, "active_portal=%d", m_data->settings.activePortal);
    XBMC->Log(LOG_DEBUG, "connection_timeout=%d", m_data->settings.connectionTimeout);

    XBMC->Log(LOG_DEBUG, "mac=%s", m_data->settings.mac.c_str());
    XBMC->Log(LOG_DEBUG, "server=%s", m_data->settings.server.c_str());
    XBMC->Log(LOG_DEBUG, "timeZone=%s", m_data->settings.timeZone.c_str());
    XBMC->Log(LOG_DEBUG, "login=%s", m_data->settings.login.c_str());
    XBMC->Log(LOG_DEBUG, "password=%s", m_data->settings.password.c_str());
    XBMC->Log(LOG_DEBUG, "guidePreference=%d", m_data->settings.guidePreference);
    XBMC->Log(LOG_DEBUG, "guideCache=%d", m_data->settings.guideCache);
    XBMC->Log(LOG_DEBUG, "guideCacheHours=%d", m_data->settings.guideCacheHours);
    XBMC->Log(LOG_DEBUG, "xmltvScope=%d", m_data->settings.xmltvScope);
    XBMC->Log(LOG_DEBUG, "xmltvPath=%s", m_data->settings.xmltvPath.c_str());
    XBMC->Log(LOG_DEBUG, "token=%s", m_data->settings.token.c_str());
    XBMC->Log(LOG_DEBUG, "serialNumber=%s", m_data->settings.serialNumber.c_str());
    XBMC->Log(LOG_DEBUG, "deviceId=%s", m_data->settings.deviceId.c_str());
    XBMC->Log(LOG_DEBUG, "deviceId2=%s", m_data->settings.deviceId2.c_str());
    XBMC->Log(LOG_DEBUG, "signature=%s", m_data->settings.signature.c_str());

    if (!m_data->ReloadSettings()) {
        ADDON_Destroy();
        m_currentStatus = ADDON_STATUS_LOST_CONNECTION;
        return m_currentStatus;
    }

    m_currentStatus = ADDON_STATUS_OK;
    return m_currentStatus;
}

void ADDON_Stop() {
}

void ADDON_Destroy() {
    XBMC->Log(LOG_DEBUG, "%s: Destroying the Stalker Client PVR Add-on", __FUNCTION__);

    SAFE_DELETE(m_data);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);

    m_currentStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_GetStatus() {
    return m_currentStatus;
}

bool ADDON_HasSettings() {
    return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet) {
    return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue) {
    return ADDON_STATUS_NEED_RESTART;
}

void ADDON_FreeSettings() {
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep() {
}

void OnSystemWake() {
}

void OnPowerSavingActivated() {
}

void OnPowerSavingDeactivated() {
}

const char *GetPVRAPIVersion(void) {
    static const char *strApiVersion = XBMC_PVR_API_VERSION;
    return strApiVersion;
}

const char *GetMininumPVRAPIVersion(void) {
    static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
    return strMinApiVersion;
}

const char *GetGUIAPIVersion(void) {
    return ""; // GUI API not used
}

const char *GetMininumGUIAPIVersion(void) {
    return ""; // GUI API not used
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities) {
    pCapabilities->bSupportsEPG = true;
    pCapabilities->bSupportsTV = true;
    pCapabilities->bSupportsChannelGroups = true;

    return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void) {
    static const char *strBackendName = "Stalker Middleware";
    return strBackendName;
}

const char *GetBackendVersion(void) {
    static const char *strBackendVersion = "Unknown";
    return strBackendVersion;
}

const char *GetConnectionString(void) {
    static const char *strConnectionString = m_data->settings.server.c_str();
    return strConnectionString;
}

const char *GetBackendHostname(void) {
    return "";
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd) {
    if (!m_data)
        return PVR_ERROR_SERVER_ERROR;

    return m_data->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelGroupsAmount(void) {
    if (!m_data)
        return -1;

    return m_data->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio) {
    if (!m_data)
        return PVR_ERROR_SERVER_ERROR;

    return m_data->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group) {
    if (!m_data)
        return PVR_ERROR_SERVER_ERROR;

    return m_data->GetChannelGroupMembers(handle, group);
}

int GetChannelsAmount(void) {
    if (!m_data)
        return 0;

    return m_data->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio) {
    if (!m_data)
        return PVR_ERROR_SERVER_ERROR;

    return m_data->GetChannels(handle, bRadio);
}

const char *GetLiveStreamURL(const PVR_CHANNEL &channel) {
    const char *url = "";

    if (m_data)
        url = m_data->GetChannelStreamURL(channel);

    return url;
}

unsigned int GetChannelSwitchDelay(void) {
    return 0;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook,
                       const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingsAmount(bool deleted) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UndeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording,
                                         int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING &, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenLiveStream(const PVR_CHANNEL &channel) { return false; }
void CloseLiveStream(void) { }
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return -1; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
bool SwitchChannel(const PVR_CHANNEL &channel) { return false; }
PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) { }
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return -1; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return -1; }
void DemuxReset(void) { }
void DemuxAbort(void) { }
void DemuxFlush(void) { }
DemuxPacket *DemuxRead(void) { return NULL; }
bool CanPauseStream(void) { return false; }
bool CanSeekStream(void) { return false; }
void PauseStream(bool bPaused) { }
bool SeekTime(int time, bool backwards, double *startpts) { return false; }
void SetSpeed(int speed) { }
bool IsTimeshifting(void) { return false; }
bool IsRealTimeStream(void) { return true; }
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }

}
