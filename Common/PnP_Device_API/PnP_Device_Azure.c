/**
 * \file
 * \brief API implementation for the device of the WE IoT design kit.
 *
 * \copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * \page License
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED “AS IS”. YOU ACKNOWLEDGE THAT WÜRTH ELEKTRONIK
 * EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY KIND RELATED TO, BUT NOT LIMITED
 * TO THE NON-INFRINGEMENT OF THIRD PARTIES’ INTELLECTUAL PROPERTY RIGHTS OR THE
 * MERCHANTABILITY OR FITNESS FOR YOUR INTENDED PURPOSE OR USAGE. WÜRTH ELEKTRONIK EISOS DOES NOT
 * WARRANT OR REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY PATENT
 * RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY RIGHT RELATING TO ANY
 * COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT IS USED. INFORMATION PUBLISHED BY
 * WÜRTH ELEKTRONIK EISOS REGARDING THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE
 * FROM WÜRTH ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR ENDORSEMENT
 * THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS PACKAGE
 */

#include <string.h>

#include "json-builder.h"
#include "PnP_Common_Device.h"
#include "PnP_Device_Azure.h"
#include "time.h"

#define MAX_PACKET_LOSS 3

// Serial Ports
//------Debug
extern TypeSerial *SerialDebug;
//------Calypso
extern TypeHardwareSerial *SerialCalypso;

// Radio Module
//------Calpyso
extern CALYPSO *calypso;

// Sensors
//------WSEN_PADS
extern PADS *sensorPADS;
//------WSEN_ITDS
extern ITDS *sensorITDS;
//------WSEN_TIDS
extern TIDS *sensorTIDS;
//------WSEN_HIDS
extern HIDS *sensorHIDS;

extern bool sensorsPresent;
extern bool deviceProvisioned;
extern bool deviceConfigured;

extern volatile unsigned long telemetrySendInterval;

extern uint8_t packetLost;

extern char kitID[DEVICE_CREDENTIALS_MAX_LEN];
char scopeID[DEVICE_CREDENTIALS_MAX_LEN] = {0};
extern char modelID[DEVICE_CREDENTIALS_MAX_LEN];
extern char awsendpoint[AWS_ENDPOINT_MAX_LEN];
uint8_t modeOfOperation = AWS_CONFIG_VERSION;
char iotHubAddress[MAX_URL_LEN] = {0};
char dpsServerAddress[MAX_URL_LEN] = {0};
extern uint16_t kitIDLength;
uint16_t iotHubAddrLen = 0;
extern uint8_t reqID;
extern float lastBattVolt;

static char displayText[128];
extern char pubtopic[128];
#define MAX_PAYLOAD_LENGTH 1024
static char sensorPayload[MAX_PAYLOAD_LENGTH];

// Certificates
const char *rootCACert = BALTIMORE_CYBERTRUST_ROOT_CERT;
// const char *rootCACert1 = DIGICERT_GLOBAL_ROOT_G2_CERT;
const char *rootCACert1 = STARFIELD_CALYPSO_ROOT;
const char *deviceCert = DEVICE_CERT;
const char *deviceKey = DEVICE_KEY;
const char *configuration = CONFIGURATION_DATA_2;

static bool Device_loadConfiguration();
static char *Device_SerializeData();
static char *Device_SerializeVoltageData(float voltage);
static char *Device_SerializeSendInterval(uint16_t val, uint16_t ac, uint16_t av, char *ad);

static bool Device_PublishRegReq();
static char *Device_SerializeProvReq();
static bool Device_PublishProvStatusReq(char *operationID);

static json_value *Device_GetCloudResponse();
static void removeChar(char *s, char c);
static void Device_PublishVoltage();
static void Device_PublishMACAddress();
static void Device_PublishUDID();
static void Device_PublishSWVersion();

static void Device_PublishSendInterval(uint16_t val, uint16_t ac, uint16_t av, char *ad);
static void Device_PublishDirectCmdResponse(int status, int requestID);
/**
 * @brief  Initialize all components of a device
 * @param  Debug Debug port
 * @param  CalypsoSerial Calypso serial port
 * @retval Serial debug port
 */
TypeSerial *Azure_Device_init(void *Debug, void *CalypsoSerial)
{
    Azure_Device_writeConfigFiles();
    sprintf(displayText, "Loading configuration...");
    SH1107_Display(1, 0, 24, displayText);
    if (Device_loadConfiguration() == true)
    {
        deviceConfigured = true;
        sprintf(displayText, "Connecting to Wi-Fi..");
        SH1107_Display(1, 0, 24, displayText);
        Azure_Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Loading config file failed\r\n");
    }

    if (Calypso_fileExists(calypso, DEVICE_IOT_HUB_ADDRESS))
    {
        Calypso_readFile(calypso, DEVICE_IOT_HUB_ADDRESS, (char *)iotHubAddress, MAX_URL_LEN, &iotHubAddrLen);
        deviceProvisioned = true;
    }
    else
    {
        deviceProvisioned = false;
        memset(iotHubAddress, 0, MAX_URL_LEN);
        iotHubAddrLen = MAX_URL_LEN;
    }
    return SerialDebug;
}

/**
 * @brief Function to run on completion of device configuration
 *
 * @return true
 * @return false
 */
bool Azure_Device_ConfigurationComplete()
{
    if (!Calypso_simpleInit(calypso))
    {
        SSerial_printf(SerialDebug, "Calypso init failed \r\n");
        return false;
    }
    if (Device_loadConfiguration() == true)
    {
        deviceConfigured = true;
        Azure_Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Loading config file failed\r\n");
        return false;
    }

    if (Calypso_fileExists(calypso, DEVICE_IOT_HUB_ADDRESS))
    {
        Calypso_readFile(calypso, DEVICE_IOT_HUB_ADDRESS, (char *)iotHubAddress, MAX_URL_LEN, &iotHubAddrLen);
        deviceProvisioned = true;
    }
    return deviceConfigured;
}

/**
 * @brief Load device configuration from the stored file
 *
 * @return true - Config load success
 * @return false - Config load failed
 */
static bool Device_loadConfiguration()
{
    char configBuf[512];
    uint16_t len;
    if (!Calypso_fileExists(calypso, CONFIG_FILE_PATH))
    {
        return false;
    }
    SSerial_printf(SerialDebug, "Load Config File\r\n");
    if (Calypso_readFile(calypso, CONFIG_FILE_PATH, (char *)configBuf, 512, &len))
    {
        json_value *configuration = json_parse(configBuf, len);
        if (configuration == NULL)
        {
            SSerial_printf(SerialDebug, "Unable to parse config file\r\n");
            return false;
        }
        if (AZURE_IOT_PNP_CONFIG_VERSION == configuration->u.object.values[0].value->u.integer)
        {
            modeOfOperation = AZURE_IOT_PNP_CONFIG_VERSION;
            SSerial_printf(SerialDebug, "Loading Azure Config\r\n");
            strcpy(kitID, configuration->u.object.values[1].value->u.string.ptr);
            strcpy(scopeID, configuration->u.object.values[2].value->u.string.ptr);
            strcpy(dpsServerAddress, configuration->u.object.values[3].value->u.string.ptr);
            strcpy(modelID, configuration->u.object.values[4].value->u.string.ptr);
            strcpy(calypso->settings.sntpSettings.server, configuration->u.object.values[5].value->u.string.ptr);
            strcpy(calypso->settings.sntpSettings.timezone, configuration->u.object.values[6].value->u.string.ptr);
            strcpy(calypso->settings.wifiSettings.SSID, configuration->u.object.values[7].value->u.string.ptr);
            strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configuration->u.object.values[8].value->u.string.ptr);
            calypso->settings.wifiSettings.securityParams.securityType = configuration->u.object.values[9].value->u.integer;
        }

        if (AWS_CONFIG_VERSION == configuration->u.object.values[0].value->u.integer)
        {
            modeOfOperation = AWS_CONFIG_VERSION;
            SSerial_printf(SerialDebug, "Loading Web Service Config\r\n");
            strcpy(kitID, configuration->u.object.values[1].value->u.string.ptr);
            strcpy(awsendpoint, configuration->u.object.values[2].value->u.string.ptr);
            strcpy(calypso->settings.sntpSettings.server, configuration->u.object.values[3].value->u.string.ptr);
            strcpy(calypso->settings.sntpSettings.timezone, configuration->u.object.values[4].value->u.string.ptr);
            strcpy(calypso->settings.wifiSettings.SSID, configuration->u.object.values[5].value->u.string.ptr);
            strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configuration->u.object.values[6].value->u.string.ptr);
            calypso->settings.wifiSettings.securityParams.securityType = configuration->u.object.values[7].value->u.integer;
        }

        else
        {
            SSerial_printf(SerialDebug, "Wrong config file version\r\n");
            return false;
        }
    }

    // MQTT Settings
    calypso->settings.mqttSettings.flags = ATMQTT_CREATE_FLAGS_URL | ATMQTT_CREATE_FLAGS_SEC;
    calypso->settings.mqttSettings.serverInfo.port = MQTT_PORT_SECURE;

    calypso->settings.mqttSettings.secParams.securityMethod = ATMQTT_SECURITY_METHOD_TLSV1_2;
    calypso->settings.mqttSettings.secParams.cipher = ATMQTT_CIPHER_TLS_RSA_WITH_AES_128_CBC_SHA256;
    strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_1_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.certificateFile, DEVICE_CERT_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.privateKeyFile, DEVICE_KEY_PATH);

    calypso->settings.mqttSettings.connParams.protocolVersion = ATMQTT_PROTOCOL_v3_1_1;
    calypso->settings.mqttSettings.connParams.blockingSend = 0;
    calypso->settings.mqttSettings.connParams.format = Calypso_DataFormat_Base64;
    return true;
}

/**
 * @brief  Write config files to calypso
 * @retval None
 */
void Azure_Device_writeConfigFiles()
{
    Azure_Device_disconnect_WiFi();

    if (!Calypso_writeFile(calypso, ROOT_CA_PATH, rootCACert, strlen(rootCACert)))
    {
        SSerial_printf(SerialDebug, "Unable to write root CA Certificate\r\n");
    }
    if (!Calypso_writeFile(calypso, ROOT_CA_1_PATH, rootCACert1, strlen(rootCACert1)))
    {
        SSerial_printf(SerialDebug, "Unable to write root CA 1 Certificate\r\n");
    }
    if (!Calypso_writeFile(calypso, DEVICE_CERT_PATH, deviceCert, strlen(deviceCert)))
    {
        SSerial_printf(SerialDebug, "Unable to write device Certificate\r\n");
    }
    if (!Calypso_writeFile(calypso, DEVICE_KEY_PATH, deviceKey, strlen(deviceKey)))
    {
        SSerial_printf(SerialDebug, "Unable to write device key\r\n");
    }
    if (!Calypso_writeFile(calypso, CONFIG_FILE_PATH, configuration, strlen(configuration)))
    {
        SSerial_printf(SerialDebug, "Unable to configuration data\r\n");
    }
}

/**
 * @brief  Restart MCU
 * @retval None
 */
void Azure_Device_restart()
{
    soft_reset();
}

/**
 * @brief  Check if the status of the GW is OK
 * @retval true if OK false otherwise
 */
bool Azure_Device_isStatusOK()
{
    if (calypso->status == calypso_error)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * @brief  Check if the device is connected to the Wi-Fi network
 * @retval True if connected false otherwise
 */
bool Azure_Device_isConnectedToWiFi()
{
    if (Calypso_isIPConnected(calypso))
    {
        return true;
    }
    SSerial_printf(SerialDebug, "Device not connected to Wi-Fi\r\n");
    return false;
}

/**
 * @brief  Check if the device is up to date
 * @retval True or false
 */
bool Azure_Device_isUpToDate()
{
    char versionStr[20];
    const char dot[2] = ".";
    char *majorVer, *minorVer;

    strcpy(versionStr, calypso->firmwareVersion);

    majorVer = strtok(versionStr, dot);
    minorVer = strtok(NULL, dot);

    if ((atoi(majorVer)) >= CALYPSO_FIRMWARE_MIN_MAJOR_VERSION)
    {
        if ((atoi(minorVer)) >= CALYPSO_FIRMWARE_MIN_MINOR_VERSION)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief  Check if the device is provisioned
 * @retval True if provisioned false otherwise
 */
bool Azure_Device_isProvisioned()
{
    return deviceProvisioned;
}

/**
 * @brief Check if the device is configured
 *
 * @return true Device is configured
 * @return false Device is not configured
 */
bool Azure_Device_isConfigured()
{
    return deviceConfigured;
}
/**
 * @brief  Publish provisioning request and get/save access token
 * @retval True if successful false otherwise
 */
bool Azure_Device_provision()
{
    bool ret = false;
    const char equals[2] = "=";
    char *retryAfter;
    int retryAfterSec = 0;
    if (modeOfOperation == AWS_CONFIG_VERSION)
    {
    }

    else if (modeOfOperation == AZURE_IOT_PNP_CONFIG_VERSION)
    {
        strcpy(calypso->settings.mqttSettings.clientID, kitID);
        sprintf(calypso->settings.mqttSettings.userOptions.userName, "%s/registrations/%s/api-version=2021-06-01&model-id=%s", scopeID, kitID, modelID);
        strcpy(calypso->settings.mqttSettings.serverInfo.address, dpsServerAddress);

        if (!Calypso_fileExists(calypso, ROOT_CA_PATH))
        {
            sprintf(displayText, "Error:Root CA \r\nnot found");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(SerialDebug, "Root CA not found\r\n");
            return false;
        }

        if (!Calypso_fileExists(calypso, DEVICE_CERT_PATH))
        {
            sprintf(displayText, "Error:Device cert \r\nnot found");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(SerialDebug, "Device certificate not found\r\n");
            return false;
        }

        if (!Calypso_fileExists(calypso, DEVICE_KEY_PATH))
        {
            sprintf(displayText, "Error:Device key \r\nnot found");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(SerialDebug, "Device key not found\r\n");
            return false;
        }

        if (Calypso_MQTTconnect(calypso) == false)
        {

            Calypso_MQTTDisconnect(calypso);
            /*New root CA is not yet active*/
            // Set the root CA path to the old root CA
            strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);
            /*Retry connection to MQTT*/
            Calypso_MQTTconnect(calypso);
        }

        if (calypso->status == calypso_MQTT_connected)
        {
            sprintf(displayText, "Connected to DPS");
            SH1107_Display(1, 0, 24, displayText);
            ATMQTT_subscribeTopic_t provResp;
            provResp.QoS = ATMQTT_QOS_QOS1;
            strcpy(provResp.topicString, PROVISIONING_RESP_TOPIC);

            if (!Calypso_subscribe(calypso, 0, 1, &provResp))
            {
                SSerial_printf(SerialDebug, "Unable to subscribe to topic\r\n");
            }

            if (Device_PublishRegReq())
            {
                json_value *provResponse = Device_GetCloudResponse();
                bool provDone = false;
                if (provResponse != NULL)
                {
                    strtok(calypso->topicName.data, equals);
                    strtok(NULL, equals);
                    retryAfter = strtok(NULL, equals);
                    retryAfterSec = atoi(retryAfter);
                    while (!provDone)
                    {
                        if ((retryAfterSec > 1) && (retryAfterSec < 10))
                        {
                            SSerial_printf(SerialDebug, "Retry after %is\r\n", atoi(retryAfter));
                            delay(atoi(retryAfter) * 1000);
                        }
                        else
                        {
                            SSerial_printf(SerialDebug, "5s\r\n");
                            delay(5000);
                        }

                        Device_PublishProvStatusReq(provResponse->u.object.values[0].value->u.string.ptr);
                        json_value *provResponse = Device_GetCloudResponse();
                        SSerial_printf(SerialDebug, "%s\r\n", provResponse->u.object.values[1].value->u.string.ptr);
                        strtok(calypso->topicName.data, equals);
                        strtok(NULL, equals);
                        retryAfter = strtok(NULL, equals);
                        if (0 == strncmp(provResponse->u.object.values[1].value->u.string.ptr, "assigned", strlen("assigned")))
                        {
                            provDone = true;
                            strcpy(iotHubAddress, provResponse->u.object.values[2].value->u.object.values[3].value->u.string.ptr);
                            if (Calypso_writeFile(calypso, DEVICE_IOT_HUB_ADDRESS, iotHubAddress, strlen(iotHubAddress)))
                            {
                                deviceProvisioned = true;
                                ret = true;
                                sprintf(displayText, "Provisioning complete");
                                SH1107_Display(1, 0, 24, displayText);
                                SSerial_printf(SerialDebug, "Provisioning done: Connect to IoT hub %s\r\n", iotHubAddress);
                            }
                        }
                        else if (0 == strncmp(provResponse->u.object.values[1].value->u.string.ptr, "failed", strlen("failed")))
                        {
                            provDone = true;
                            ret = false;
                            sprintf(displayText, "Provisioning failed");
                            SH1107_Display(1, 0, 24, displayText);
                            SSerial_printf(SerialDebug, "Provisioning failed\r\n");
                        }
                    }

                    json_value_free(provResponse);
                }
                else
                {
                    SSerial_printf(SerialDebug, "Response empty\r\n");
                }
            }
            Calypso_MQTTDisconnect(calypso);
        }
        else
        {
            sprintf(displayText, "Error: Uanable to connect\r\n to DPS");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(SerialDebug, "Unable to connect to MQTT server \r\n");
        }
    }

    return ret;
}

void Azure_Device_configurationInProgress()
{
    char temp[20];
    static uint8_t state;
    memcpy(temp, calypso->MAC_ADDR, 20);
    removeChar(temp, ':');
    SSerial_printf(SerialDebug, "Device in configuration mode\r\n");
    SSerial_printf(SerialDebug, "Connect to the Calypso Access point to configure the device\r\n");
    SSerial_printf(SerialDebug, "Calypso Access point SSID is calypso_%s\r\n", temp);
    SSerial_printf(SerialDebug, "Restart the device after configuration\r\n");

    switch (state)
    {
    case 0:
        sprintf(displayText, "** Config mode **\r\n\r\nPerform the \r\nfollowing 5 steps\r\n\r\n\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 1:
        sprintf(displayText, "** Config mode **\r\n\r\n1. Connect your PC to\r\nthe access point\r\n\r\ncalypso_%s\r\n\r\n--->", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 2:
        sprintf(displayText, "** Config mode **\r\n\r\n2. Open your browser\r\n\r\n\r\n\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 3:
        sprintf(displayText, "** Config mode **\r\n\r\n3. Navigate to page\r\n\r\ncalypso.net/azure.html\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 4:
        sprintf(displayText, "** Config mode **\r\n\r\n4. Select and upload \r\nthe config files\r\n\r\n\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 5:
        sprintf(displayText, "** Config mode **\r\n\r\n5. Upon completion,\r\n\r\nPress reset button \r\non device");
        SH1107_Display(1, 0, 0, displayText);
        state = 0;
        break;
    default:
        break;
    }
}

/**
 * @brief  Collect data from sensors connected to the device
 * @retval None
 */
void Azure_Device_readSensors()
{
    if (!PADS_readSensorData(sensorPADS))
    {
        SSerial_printf(SerialDebug, "Error reading pressure data\r\n");
    }

    if (!ITDS_readSensorData(sensorITDS))
    {
        SSerial_printf(SerialDebug, "Error reading acceleration data\r\n");
    }

    if (!TIDS_readSensorData(sensorTIDS))
    {
        SSerial_printf(SerialDebug, "Error reading temperature data\r\n");
    }

    if (!HIDS_readSensorData(sensorHIDS))
    {
        SSerial_printf(SerialDebug, "Error reading humidity data\r\n");
    }
}

/**
 * @brief  Connect GW to cloud MQTT sever
 * @retval None
 */
void Azure_Device_MQTTConnect()
{
    if (modeOfOperation == AWS_CONFIG_VERSION)
    {
        strcpy(calypso->settings.mqttSettings.clientID, kitID);
        strcpy(calypso->settings.mqttSettings.serverInfo.address, awsendpoint);
        if (Calypso_MQTTconnect_AWS(calypso) == false)
        {
            if (calypso->status == calypso_MQTT_wrong_root_ca)
            {
                Calypso_MQTTDisconnect(calypso);
                /*New root CA is not yet active*/
                // Set the root CA path to the old root CA
                strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);
                /*Retry connection to MQTT*/
                if (!Calypso_MQTTconnect_AWS(calypso))
                {
                    sprintf(displayText, "Error: Failed to \r\nconnect to \r\nAWS");
                    SH1107_Display(1, 0, 24, displayText);
                }
                else
                {
                    sprintf(displayText, "Connected to \r\nAWS");
                    SH1107_Display(1, 0, 24, displayText);
                }
            }
        }
        else
        {
            sprintf(displayText, "Error: Failed to connect\r\nto AWS");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(calypso->serialDebug, "MQTT connect fail\r\n");
        }
    }

    if (modeOfOperation == AZURE_IOT_PNP_CONFIG_VERSION)
    {
        strcpy(calypso->settings.mqttSettings.clientID, kitID);
        sprintf(calypso->settings.mqttSettings.userOptions.userName, "%s/%s/?api-version=2021-04-12&model-id=%s", iotHubAddress, kitID, modelID);
        strcpy(calypso->settings.mqttSettings.serverInfo.address, iotHubAddress);
        if (Calypso_MQTTconnect(calypso) == false)
        {
            if (calypso->status == calypso_MQTT_wrong_root_ca)
            {
                Calypso_MQTTDisconnect(calypso);
                /*New root CA is not yet active*/
                // Set the root CA path to the old root CA
                strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);
                /*Retry connection to MQTT*/
                if (!Calypso_MQTTconnect(calypso))
                {
                    sprintf(displayText, "Error: Failed to \r\nconnect to \r\nIoT central");
                    SH1107_Display(1, 0, 24, displayText);
                }
                else
                {
                    sprintf(displayText, "Connected to \r\nIoT central");
                    SH1107_Display(1, 0, 24, displayText);
                }
            }
        }
        else
        {
            sprintf(displayText, "Error: Failed to connect\r\nto IoT central");
            SH1107_Display(1, 0, 24, displayText);
            SSerial_printf(calypso->serialDebug, "MQTT connect fail\r\n");
        }
    }
}

/**
 * @brief  Subscribe to topics
 * @retval True if successful false otherwise
 */
bool Azure_Device_SubscribeToTopics()
{
    if (calypso->status == calypso_MQTT_connected)
    {
        ATMQTT_subscribeTopic_t topics[4];
        ATMQTT_subscribeTopic_t twinRes, twinDesiredResp, directMethod;

        twinRes.QoS = ATMQTT_QOS_QOS1;
        strcpy(twinRes.topicString, DEVICE_TWIN_RES_TOPIC);

        twinDesiredResp.QoS = ATMQTT_QOS_QOS1;
        strcpy(twinDesiredResp.topicString, DEVICE_TWIN_DESIRED_PROP_RES_TOPIC);

        directMethod.QoS = ATMQTT_QOS_QOS1;
        strcpy(directMethod.topicString, DIRECT_METHOD_TOPIC);

        topics[0] = twinRes;
        topics[1] = twinDesiredResp;
        topics[2] = directMethod;
        return (Calypso_subscribe(calypso, 0, 3, topics));
    }
    else
    {
        return false;
    }
}

/**
 * @brief  Publish device provision request
 * @retval true if successful false otherwise
 */
static bool Device_PublishRegReq()
{
    bool ret = false;
    char provReqTopic[128];

    reqID++;
    sprintf(provReqTopic, "%s%u", PROVISIONING_REG_REQ_TOPIC, reqID);

    char *provReq = Device_SerializeProvReq();

    if (!Calypso_MQTTPublishData(calypso, provReqTopic, 1, provReq, strlen(provReq), true))
    {
        ret = false;
        SSerial_printf(SerialDebug, "Provision Publish failed\n\r");
    }
    else
    {
        ret = true;
    }
    return ret;
}

/**
 * @brief  Publish device provision request
 * @param  OpertionID
 * @retval true if successful false otherwise
 */
static bool Device_PublishProvStatusReq(char *operationID)
{
    bool ret = false;
    char provStatusTopic[256];
    char *payload = 0;

    reqID++;
    sprintf(provStatusTopic, "%s%u&operationId=%s", PROVISIONING_STATUS_REQ_TOPIC, reqID, operationID);

    if (!Calypso_MQTTPublishData(calypso, provStatusTopic, 1, payload, 0, true))
    {
        ret = false;
        SSerial_printf(SerialDebug, "Provision Publish failed\n\r");
    }
    else
    {
        ret = true;
    }
    return ret;
}

/**
 * @brief  Wait for response from the cloud
 * @retval JSON message or NULL
 */
static json_value *Device_GetCloudResponse()
{
    json_value *response = NULL;
    if ((Calypso_MQTTgetMessage(calypso, true)) && (calypso->bufferCalypso.length > 4))
    {
        response = json_parse(calypso->bufferCalypso.data, calypso->bufferCalypso.length);
        memset(calypso->bufferCalypso.data, 0, CALYPSO_LINE_MAX_SIZE);
        calypso->bufferCalypso.length = 0;
    }
    else
    {
        // SSerial_printf(SerialDebug, "No message\r\n");
    }
    return response;
}

/**
 * @brief  Publish the values of sensors connected to the device
 * @retval None
 */
void Azure_Device_PublishSensorData()
{
    Azure_Device_readSensors();
    char *dataSerialized = Device_SerializeData();
#if SERIAL_DEBUG
    // SSerial_writeB(SerialDebug, dataSerialized, strlen(dataSerialized));
    // SSerial_printf(SerialDebug, "\r\n");
#endif
    pubtopic[0] = '\0';
    sprintf(pubtopic, "devices/%s/messages/events/", kitID);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, dataSerialized, strlen(dataSerialized), true))
    {
        packetLost++;
        SSerial_printf(SerialDebug, "Publish failed %u\r\n", packetLost);
        if (packetLost == MAX_PACKET_LOSS)
        {
            calypso->status = calypso_error;
        }
    }
}

/**
 * @brief  Display sensor data on OLED display
 * @retval None
 */
void Azure_Device_displaySensorData()
{
    sprintf(displayText, "Status: Connected\r\nP:%0.2f kPa\r\nT:%0.2f C\r\nRH:%0.2f %%\r\nAcc: x:%0.2f g\r\n     y:%0.2f g\r\n     z:%0.2f g",
            sensorPADS->data[0],
            sensorTIDS->data[0],
            sensorHIDS->data[0],
            sensorITDS->data[0],
            sensorITDS->data[1],
            sensorITDS->data[2]);
    SH1107_Display(1, 0, 0, displayText);
}

/**
 * @brief  Publish the device voltage (Read-only property)
 * @retval None
 */
static void Device_PublishVoltage()
{
    float currentVoltage = getBatteryVoltage();

    char *dataSerializedVolt = Device_SerializeVoltageData(currentVoltage);

    reqID++;
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_MESSAGE_PATCH, reqID);

    SSerial_printf(SerialDebug, "%s\r\n", dataSerializedVolt);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, dataSerializedVolt, strlen(dataSerializedVolt), true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 8, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nBatt voltage: %0.2f V", currentVoltage);
        SH1107_Display(1, 0, 8, displayText);
    }

    lastBattVolt = currentVoltage;
}
static void Device_PublishSWVersion()
{
    json_value *fwVersion = json_object_new(2);
    json_object_push(fwVersion, "__t", json_string_new("c"));
    json_object_push(fwVersion, "swVersion", json_string_new(calypso->firmwareVersion));
    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", fwVersion);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);
    json_builder_free(payload);
    json_builder_free(fwVersion);

    reqID++;
    pubtopic[0] = '\0';
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_MESSAGE_PATCH, reqID);

    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, sensorPayload, strlen(sensorPayload), true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nVersion:%s ", calypso->firmwareVersion);
        SH1107_Display(1, 0, 8, displayText);
    }
}
static void Device_PublishUDID()
{
    json_value *jsonUdid = json_object_new(2);
    json_object_push(jsonUdid, "__t", json_string_new("c"));
    json_object_push(jsonUdid, "udid", json_string_new(calypso->udid));

    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", jsonUdid);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);
    json_builder_free(payload);
    json_builder_free(jsonUdid);

    reqID++;
    pubtopic[0] = '\0';
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_MESSAGE_PATCH, reqID);

    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, sensorPayload, strlen(sensorPayload), true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nUDID:\r\n%s ", calypso->udid);
        SH1107_Display(1, 0, 8, displayText);
    }
}
static void Device_PublishMACAddress()
{
    json_value *macAddr = json_object_new(2);
    json_object_push(macAddr, "__t", json_string_new("c"));
    json_object_push(macAddr, "macAddress", json_string_new(calypso->MAC_ADDR));

    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", macAddr);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);
    json_builder_free(payload);
    json_builder_free(macAddr);

    reqID++;
    pubtopic[0] = '\0';
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_MESSAGE_PATCH, reqID);

    SSerial_printf(SerialDebug, "%s\r\n", sensorPayload);

    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, sensorPayload, strlen(sensorPayload), true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nMAC ADDRESS:\r\n%s ", calypso->MAC_ADDR);
        SH1107_Display(1, 0, 8, displayText);
    }
}

/**
 * @brief  Publish the values of sensors connected to the device
 * @retval None
 */
void Azure_Device_PublishProperties()
{

    /*Request the desired values for writable properties*/
    reqID++;
    pubtopic[0] = '\0';
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_GET_TOPIC, reqID);

    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, NULL, 0, true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
    }

    /*Process the response*/
    Azure_Device_processCloudMessage();

    Device_PublishVoltage();
    Azure_Device_processCloudMessage();

    Device_PublishMACAddress();
    Azure_Device_processCloudMessage();

    Device_PublishUDID();
    Azure_Device_processCloudMessage();

    Device_PublishSWVersion();
    Azure_Device_processCloudMessage();
}

/**
 * @brief  Publish response to direct command
 * @retval None
 */
static void Device_PublishDirectCmdResponse(int status, int requestID)
{
    pubtopic[0] = '\0';
    sprintf(pubtopic, "$iothub/methods/res/%i/?$rid=%i", status, requestID);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, NULL, 0, true))
    {
        SSerial_printf(SerialDebug, "Publish method response failed\r\n");
    }
}

/**
 * @brief  Publish the set send interval (Writable property)
 * @retval None
 */
static void Device_PublishSendInterval(uint16_t val, uint16_t ac, uint16_t av, char *ad)
{
    reqID++;
    pubtopic[0] = '\0';
    sprintf(pubtopic, "%s%u", DEVICE_TWIN_MESSAGE_PATCH, reqID);
    char *dataSerializedInterval = Device_SerializeSendInterval(val, ac, av, ad);
    SSerial_printf(SerialDebug, "%s\r\n", dataSerializedInterval);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, dataSerializedInterval, strlen(dataSerializedInterval), true))
    {
        SSerial_printf(SerialDebug, "Properties Publish failed\r\n");
    }
}
/**
 * @brief  Process messages from the cloud
 * @retval None
 */
void Azure_Device_processCloudMessage()
{
    json_value *cloudResponse = NULL;
    const char backsSlash[2] = "/";
    const char equals[2] = "=";
    char *token;
    char *reqIDstr;
    cloudResponse = Device_GetCloudResponse();

    if (strstr(calypso->topicName.data, "$iothub/twin/res/"))
    {

        strtok(calypso->topicName.data, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        token = strtok(NULL, backsSlash);

        if (atoi(token) == STATUS_CLOUD_SUCCESS)
        {
            SSerial_printf(SerialDebug, "Device property updated successfully!\r\n");
        }
        else if (atoi(token) == STATUS_SUCCESS)
        {
            unsigned long desiredVal = 0;
            uint16_t version = 0;
            /*Received response for the properties get request*/
            if (0 == strncmp(cloudResponse->u.object.values[0].value->u.object.values[0].name, "telemetrySendFrequency", strlen("telemetrySendFrequency")))
            {
                desiredVal = (unsigned long)cloudResponse->u.object.values[0].value->u.object.values[0].value->u.integer;
                version = (unsigned long)cloudResponse->u.object.values[0].value->u.object.values[1].value->u.integer;
                /*Defualt value set by the cloud */
                if ((desiredVal > MAX_TELEMETRY_SEND_INTERVAL) || (desiredVal < MIN_TELEMETRY_SEND_INTERVAL))
                {
                    // value out of range, send response
                    Device_PublishSendInterval(desiredVal, STATUS_BAD_REQUEST, version, "invalid parameter");
                }
                else
                {
                    // set the value
                    telemetrySendInterval = desiredVal * 1000;
                    Device_PublishSendInterval(desiredVal, STATUS_SUCCESS, version, "success");

                    sprintf(displayText, "Property updated\r\nsend interval: %lu s", desiredVal);
                    SH1107_Display(1, 0, 24, displayText);
                }
            }
            else
            {
                /*No default value available, setting the value from the device*/
                version = (unsigned long)0;
                desiredVal = (unsigned long)DEFAULT_TELEMETRY_SEND_INTEVAL;
                Device_PublishSendInterval(desiredVal, STATUS_SET_BY_DEV, version, "initialize");
            }
        }
        else
        {
            SSerial_printf(SerialDebug, "Request failed error:%i\r\n", atoi(token));
        }
    }
    else if (strstr(calypso->topicName.data, "$iothub/twin/PATCH/properties/desired/"))
    {
        /*Request to update writable property from cloud*/
        unsigned long desiredVal = (unsigned long)cloudResponse->u.object.values[0].value->u.integer;
        uint16_t version = (uint16_t)cloudResponse->u.object.values[1].value->u.integer;

        SSerial_printf(SerialDebug, "desired val %i, version %i\r\n", desiredVal, version);
        if ((desiredVal > MAX_TELEMETRY_SEND_INTERVAL) || (desiredVal < MIN_TELEMETRY_SEND_INTERVAL))
        {
            // value out of range, send response
            Device_PublishSendInterval(desiredVal, STATUS_BAD_REQUEST, version, "invalid parameter");
        }
        else
        {
            // set the value
            telemetrySendInterval = desiredVal * 1000;
            Device_PublishSendInterval(desiredVal, STATUS_SUCCESS, version, "success");
            sprintf(displayText, "Property updated\r\nsend interval: %lu s", desiredVal);
            SH1107_Display(1, 0, 24, displayText);
        }
    }
    else if (strstr(calypso->topicName.data, "iothub/methods/POST/setLEDColor/"))
    {

        strtok(calypso->topicName.data, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        token = strtok(NULL, backsSlash);
        strtok(token, equals);
        reqIDstr = strtok(NULL, equals);

        int red = cloudResponse->u.object.values[0].value->u.integer;
        int green = cloudResponse->u.object.values[1].value->u.integer;
        int blue = cloudResponse->u.object.values[2].value->u.integer;

        /*Direct command to set LED color*/
        if ((red < 0) || (red > 0xFF) ||
            (green < 0) || (green > 0xFF) ||
            (blue < 0) || (blue > 0xFF))
        {
            // value out of range, send response
            Device_PublishDirectCmdResponse(STATUS_BAD_REQUEST, atoi(reqIDstr));
        }
        else
        {
            // value valid, set and send response
            uint32_t color = ((uint32_t)(red << 16) + (uint32_t)(green << 8) + (uint32_t)blue);
            neopixelSet(color);
            Device_PublishDirectCmdResponse(STATUS_SUCCESS, atoi(reqIDstr));
            sprintf(displayText, "LED color set\r\nR: %u\r\nG: %u\r\nB: %u", red, green, blue);
            SH1107_Display(1, 0, 16, displayText);
        }
    }
    if (cloudResponse != NULL)
    {
        json_value_free(cloudResponse);
    }
}

/**
 * @brief  Connect to WiFi
 * @retval None
 */
void Azure_Device_connect_WiFi()
{
    if (!Calypso_WLANconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi connect fail\r\n");
        sprintf(displayText, "Error: Wi-Fi connect \r\nfailed");
        SH1107_Display(1, 0, 24, displayText);
        return;
    }
    delay(WI_FI_CONNECT_DELAY);
    if (calypso->status == calypso_WLAN_connected)
    {
        sprintf(displayText, "Connected to Wi-Fi");
        SH1107_Display(1, 0, 24, displayText);
    }
    if (!Calypso_setUpSNTP(calypso))
    {
        SSerial_printf(calypso->serialDebug, "SNTP config fail\r\n");
    }
}

/**
 * @brief  Disconnect from WiFi
 * @retval None
 */
void Azure_Device_disconnect_WiFi()
{
    if (!Calypso_WLANDisconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi disconnect fail\r\n");
    }
    calypso->status = calypso_WLAN_disconnected;
}
/**
 * @brief  Reset cloud access token and claim status
 * @retval None
 */
void Azure_Device_reset()
{
    Azure_Device_disconnect_WiFi();
    /*Delete device credentials*/
    if (Calypso_fileExists(calypso, DEVICE_IOT_HUB_ADDRESS))
    {
        Calypso_deleteFile(calypso, DEVICE_IOT_HUB_ADDRESS);
    }
    if (Calypso_fileExists(calypso, CONFIG_FILE_PATH))
    {
        Calypso_deleteFile(calypso, CONFIG_FILE_PATH);
    }
    soft_reset();
}

/**
 * @brief  Start calypso provisioning
 * @retval None
 */
void Azure_Device_WiFi_provisioning()
{
    if (calypso->status == calypso_provisioning)
    {
        return;
    }
    if (Calypso_StartProvisioning(calypso))
    {
        calypso->status = calypso_provisioning;
    }
    else
    {
        SSerial_printf(calypso->serialDebug, "Start provisioning fail\r\n");
    }
}

/**
 * @brief  Serialize data to send
 * @retval Pointer to serialized data
 */
static char *Device_SerializeData()
{
    uint8_t idx = 0;
    json_value *payload = json_object_new(padsProperties + tidsProperties + hidsProperties + 1);
    if (payload == NULL)
    {
        SSerial_printf(SerialDebug, "Payload memory full \r\n");
        return NULL;
    }
    json_value *acceleration = json_object_new(3);
    if (acceleration == NULL)
    {
        json_builder_free(payload);
        SSerial_printf(SerialDebug, "acceleration memory full \r\n");
        return NULL;
    }

    for (idx = 0; idx < padsProperties; idx++)
    {
        json_object_push(payload, sensorPADS->dataNames[idx],
                         json_double_new(sensorPADS->data[idx]));
    }
    for (idx = 0; idx < hidsProperties; idx++)
    {
        json_object_push(payload, sensorHIDS->dataNames[idx],
                         json_double_new(sensorHIDS->data[idx]));
    }
    for (idx = 0; idx < tidsProperties; idx++)
    {
        json_object_push(payload, sensorTIDS->dataNames[idx],
                         json_double_new(sensorTIDS->data[idx]));
    }

    for (idx = 0; idx < itdsProperties; idx++)
    {
        json_object_push(acceleration, sensorITDS->dataNames[idx],
                         json_double_new(sensorITDS->data[idx]));
    }
    json_object_push(payload, "acceleration", acceleration);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);

    json_builder_free(payload);
    json_builder_free(acceleration);
    return sensorPayload;
}

/**
 * @brief  Serialize data to send
 * @retval Pointer to serialized data
 */
static char *Device_SerializeVoltageData(float voltage)
{
    json_value *payload = json_object_new(1);
    json_object_push(payload, "batteryVoltage", json_double_new(voltage));

    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);

    json_builder_free(payload);
    return sensorPayload;
}

/**
 * @brief  Serialize data to send
 * @retval Pointer to serialized data
 */
static char *Device_SerializeSendInterval(uint16_t val, uint16_t ac, uint16_t av, char *ad)
{
    json_value *payload = json_object_new(1);
    json_value *obj = json_object_new(1);

    json_object_push(obj, "value", json_integer_new(val));
    json_object_push(obj, "ac", json_integer_new(ac));
    json_object_push(obj, "av", json_integer_new(av));
    json_object_push(obj, "ad", json_string_new(ad));

    json_object_push(payload, "telemetrySendFrequency", obj);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);

    json_builder_free(payload);
    json_builder_free(obj);
    return sensorPayload;
}

/**
 * @brief  Serialize provision request
 * @param  devName device name
 * @param  provkey Provisioning key
 * @param  provsecret Provisioning secret
 * @retval serialized data
 */
static char *Device_SerializeProvReq()
{
    json_value *prov_payload_ = json_object_new(1);
    json_value *prov_modelID_ = json_object_new(1);

    json_object_push(prov_modelID_, "modelId",
                     json_string_new(modelID));
    json_object_push(prov_payload_, "registrationId",
                     json_string_new(kitID));
    json_object_push(prov_payload_, "payload",
                     prov_modelID_);

    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, prov_payload_);

    json_builder_free(prov_payload_);
    json_builder_free(prov_modelID_);

    return sensorPayload;
}

/**
 * @brief  Remove a specific charecter from a string
 * @param  s input string
 * @param  c char to remove
 * @retval None
 */
static void removeChar(char *s, char c)
{

    int j, n = strlen(s);
    for (int i = j = 0; i < n; i++)
        if (s[i] != c)
            s[j++] = s[i];

    s[j] = '\0';
}

unsigned long Azure_Device_getTelemetrySendInterval()
{
    return telemetrySendInterval;
}

bool Azure_Device_isSensorsPresent()
{
    return sensorsPresent;
}