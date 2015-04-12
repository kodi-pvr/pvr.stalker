/*
 *      Copyright (C) 2015  Jamal Edey
 *      http://www.kenshisoft.com/
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

#include "xbmc_pvr_dll.h"
#include "platform/util/util.h"

#include "SData.h"

using namespace ADDON;

ADDON_STATUS  m_CurStatus = ADDON_STATUS_UNKNOWN;
SData         *m_data     = NULL;

std::string g_strUserPath   = "";
std::string g_strClientPath = "";

/* User adjustable settings are saved here.
* Default values are defined inside client.h
* and exported to the other source files.
*/
std::string g_strMac          = "";
std::string g_strServer       = "";
std::string g_strTimeZone     = "";
std::string g_strApiBasePath  = "";
std::string g_api_endpoint    = "";
std::string g_referer         = "";
std::string g_token           = "";

CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR  = NULL;

extern "C" {

ADDON_STATUS ADDON_Create(void* callbacks, void* props)
{
  if (!callbacks || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrProps = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(callbacks))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(callbacks))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s: Creating the Stalker Client PVR Add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  m_data          = new SData;
  g_strUserPath   = pvrProps->strUserPath;
  g_strClientPath = pvrProps->strClientPath;

  if (!XBMC->DirectoryExists(g_strUserPath.c_str())) {
#ifdef TARGET_WINDOWS
    CreateDirectory(g_strUserPath.c_str(), NULL);
#else
    XBMC->CreateDirectory(g_strUserPath.c_str());
#endif
  }

  char buffer[1024];

  if (!XBMC->GetSetting("mac", &buffer)) {
    XBMC->Log(LOG_DEBUG, "mac address not set");
    g_strMac = DEFAULT_MAC;
  }
  else {
    g_strMac = buffer;
  }

  if (!XBMC->GetSetting("server", &buffer)) {
    XBMC->Log(LOG_DEBUG, "server address not set");
    g_strServer = DEFAULT_SERVER;
  }
  else {
    g_strServer = buffer;
  }

  if (!XBMC->GetSetting("time_zone", &buffer)) {
    XBMC->Log(LOG_DEBUG, "time zone not set");
    g_strTimeZone = DEFAULT_TIME_ZONE;
  }
  else {
    g_strTimeZone = buffer;
  }

  if (!m_data->LoadData()) {
    XBMC->QueueNotification(QUEUE_ERROR, "Startup failed.");
    ADDON_Destroy();
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
  }
  else {
    m_CurStatus = ADDON_STATUS_OK;
  }

  return m_CurStatus;
}

void ADDON_Stop()
{
}

void ADDON_Destroy()
{
  XBMC->Log(LOG_DEBUG, "%s: Destroying the Stalker Client PVR Add-on", __FUNCTION__);

  if (m_data)
    SAFE_DELETE(m_data);

  if (PVR)
    SAFE_DELETE(PVR);

  if (XBMC)
    SAFE_DELETE(XBMC);

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  return ADDON_STATUS_NEED_RESTART;
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

const char* GetPVRAPIVersion(void)
{
  static const char *strApiVersion = XBMC_PVR_API_VERSION;
  return strApiVersion;
}

const char* GetMininumPVRAPIVersion(void)
{
  static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
  return strMinApiVersion;
}

const char* GetGUIAPIVersion(void)
{
  static const char *strGuiApiVersion = XBMC_GUI_API_VERSION;
  return strGuiApiVersion;
}

const char* GetMininumGUIAPIVersion(void)
{
  static const char *strMinGuiApiVersion = XBMC_GUI_MIN_API_VERSION;
  return strMinGuiApiVersion;
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG           = true;
  pCapabilities->bSupportsTV            = true;
  pCapabilities->bSupportsChannelGroups = true;
  
  return PVR_ERROR_NO_ERROR;
}

const char* GetBackendName(void)
{
  static const char *strBackendName = "Stalker Middleware";
  return strBackendName;
}

const char* GetBackendVersion(void)
{
  static const char *strBackendVersion = "Unknown";
  return strBackendVersion;
}

const char* GetConnectionString(void)
{
  static const char *strConnectionString = g_strServer.c_str();
  return strConnectionString;
}

const char* GetBackendHostname(void)
{
  return "";
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!m_data)
    return PVR_ERROR_SERVER_ERROR;

  return m_data->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelGroupsAmount(void)
{
  if (!m_data)
    return -1;

  return m_data->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!m_data)
    return PVR_ERROR_SERVER_ERROR;

  return m_data->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!m_data)
    return PVR_ERROR_SERVER_ERROR;

  return m_data->GetChannelGroupMembers(handle, group);
}

int GetChannelsAmount(void)
{
  if (!m_data)
    return 0;

  return m_data->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!m_data)
    return PVR_ERROR_SERVER_ERROR;

  return m_data->GetChannels(handle, bRadio);
}

const char* GetLiveStreamURL(const PVR_CHANNEL& channel)
{
  const char* url = "";

  if (m_data)
    url = m_data->GetChannelStreamURL(channel);

  if (strcmp(url, "") == 0)
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to get stream URL.");

  return url;
}

unsigned int GetChannelSwitchDelay(void) {
  return 0;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingsAmount(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenLiveStream(const PVR_CHANNEL &channel) { return false; }
void CloseLiveStream(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return -1; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
int GetCurrentClientChannel(void) { return -1; }
bool SwitchChannel(const PVR_CHANNEL& channel) { return false; }
PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return -1; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return -1; }
void DemuxReset(void) {}
void DemuxAbort(void) {}
void DemuxFlush(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
bool CanPauseStream(void) { return false; }
bool CanSeekStream(void) { return false; }
void PauseStream(bool bPaused) {}
bool SeekTime(int time, bool backwards, double *startpts) { return false; }
void SetSpeed(int speed) {}
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }

}
