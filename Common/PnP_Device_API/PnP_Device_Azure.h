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

#ifndef P_N_P_DEVICE_H
#define P_N_P_DEVICE_H

/**         Includes         */

#include <stdint.h>
#include "calypsoBoard.h"
#include "ConfigPlatform.h"
#include "sensorBoard.h"

/**         Functions definition         */

#ifdef __cplusplus
extern "C"
{
#endif

#define KIT_ID "calypso-test-dev-1"
/*Root CA certificate to create the TLS connection*/
#define BALTIMORE_CYBERTRUST_ROOT_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n\
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n\
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n\
DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n\
ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n\
VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n\
mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n\
IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n\
mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n\
XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n\
dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n\
jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n\
BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n\
DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n\
9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n\
jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n\
Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n\
ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n\
R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n\
-----END CERTIFICATE-----"

#define DIGICERT_GLOBAL_ROOT_G2_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n\
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n\
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n\
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n\
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n\
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n\
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n\
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n\
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n\
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n\
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n\
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n\
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n\
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n\
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n\
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n\
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n\
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n\
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n\
MrY=\n\
-----END CERTIFICATE-----"

#define STARFIELD_CALYPSO_ROOT "-----BEGIN CERTIFICATE-----\n\
MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n\
MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n\
U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n\
NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n\
ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n\
ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n\
DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n\
8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n\
+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n\
X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n\
K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n\
1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n\
A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n\
zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n\
YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n\
bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n\
DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n\
L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n\
eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n\
xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n\
VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n\
WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n\
-----END CERTIFICATE-----"

/*Device certificate chain*/
#define DEVICE_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDWTCCAkGgAwIBAgIUaLIOklwvPlEccnHGhnnIpwE7cqUwDQYJKoZIhvcNAQEL\n\
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n\
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MTIxODA5MDgz\n\
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n\
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK4hXGazHjP1wiPGId59\n\
6UavUKGOPFM4YRpk2eHCvD8+IiyQhVBNDerHJaScSQo6wgaQGNPn55P8g88NX9BO\n\
UlOTxQFbCSvQ5Qpr9r/pBOmNfjH8ZhnIWQ5p1mkxHWvmOoVqykdsrLxY1uf5erv6\n\
QyZDV9m2E07wGTRreX7UQsDpZhoS4aip/xJ+j/rTiV18kkynml0/tgQiExqmijCE\n\
XIlWfQXfnjsBNlC72BtsmWlen0hBO6934MIZnFEE11w+SnTBwKjt2EBc0jlStdzD\n\
8DHWoao5IhZoFeyr25Bz9+oJJ2byGsnk3Ui4Kgjapl1FLVKYTAcKewFUBCWAixDA\n\
QZcCAwEAAaNgMF4wHwYDVR0jBBgwFoAU0P20EBZhuHlpi7Gul/6WRarDQWEwHQYD\n\
VR0OBBYEFAhjuTsBk03wbfoNwa0HZrqRVik/MAwGA1UdEwEB/wQCMAAwDgYDVR0P\n\
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCalSZnPHkoW2+QDbfORoajDs3v\n\
wnLaZZ+6IJ5dum/7nGbtshrgssDkzjOVt2IBDSXfuf2MTFf8UdVEPwuijRleaStW\n\
uBo5fgMu17v6qzYsNnJRZPUSigvG0zBlk9VB/8v1OYwnoEX3cAmSmmV83nN2SORy\n\
0h67J/Vw6EdzTbLSiLf558OTTac6LjwNI+gCkcd9QGTvA0MQACI/eY7aDAx+oGKH\n\
NqbotOqdNG2UDCJOlmYIqEdjbRkRT7Z1IVlfniV5dCn+TfSyHq+k4fSr9ZPal5vz\n\
yAjRO1vczK2GEHG+yTQi0ge3fn8+7XcmzBgNYxwKmFvmGcZlrxbU7sGdMTnm\n\
-----END CERTIFICATE-----"

/*Device key*/
#define DEVICE_KEY "-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpAIBAAKCAQEAriFcZrMeM/XCI8Yh3n3pRq9QoY48UzhhGmTZ4cK8Pz4iLJCF\n\
UE0N6sclpJxJCjrCBpAY0+fnk/yDzw1f0E5SU5PFAVsJK9DlCmv2v+kE6Y1+Mfxm\n\
GchZDmnWaTEda+Y6hWrKR2ysvFjW5/l6u/pDJkNX2bYTTvAZNGt5ftRCwOlmGhLh\n\
qKn/En6P+tOJXXySTKeaXT+2BCITGqaKMIRciVZ9Bd+eOwE2ULvYG2yZaV6fSEE7\n\
r3fgwhmcUQTXXD5KdMHAqO3YQFzSOVK13MPwMdahqjkiFmgV7KvbkHP36gknZvIa\n\
yeTdSLgqCNqmXUUtUphMBwp7AVQEJYCLEMBBlwIDAQABAoIBAGiAZvbPeknOrLNR\n\
fhQL1orwPeCm/vcmt8fiTIxblSQTQukh1pAJnlePKr0uefskpjrQEcZiv60ld2k0\n\
apMV3fyAi1Oz8b4VANAPWSd1TdhobRrMke3ZOfEXfXDl4/VUVzyoiTryMnxaiKbx\n\
J0JXACOfeMKUrePK3iWCdoiyFFm2+2xpqjsbcRIhwTPr7BZPFGtXYLNu1PKQzTqI\n\
ElIECsLPY1MWQtN5QUpcK31SMBKfPylzuO8eCX9Wgyb7BKJfjPbYS7XxqqKdsrMy\n\
O5X/Uf1sVhKqdD+3uGsUuRf8mm7Ba9Byzh+80mBP2pOmICeqWWgBkP0VGinN9naP\n\
MdtSe+ECgYEA2DuuItN0h/lMIzsCLp06VJxv3moEWGeCppbv3iOpSJ3ap5NaUyKy\n\
Qy5Wnx7h1XuA4rYVHKVov5dZPzhEtS/9kDeES14ba0xCfDVPfkcT+RYh8htPTO6+\n\
skN05lCxiRXj7Khuimr8pCxgTB1KWXZvvONi5K1AhW5sT5Z5cTC4GocCgYEAzid3\n\
yu2D21ZwomJoh3wizGYdm3qI3TwmaZirjk+Hug86uzUAal8rel1tgmWQiutWsTIJ\n\
h9iUNKpENeCpnYa34g0k991ufcVE+uEZd0C9p0qvRGV30vDYIOhMMf3b+JEePeOJ\n\
7rGQQU3zx+W9fHaazOdKbxKRf/Ps5CDpvk/GFHECgYALGxWYiE/F5BH7BT7Zcg3a\n\
5qYAQGW0vKxDLiFnwWEib6kZTkInXvLU7H5acdWbh1pZSozPCdfVb0qQKq2suKhH\n\
TfKnhE/YNPR9OKe6jqAB/RcFPk3WX7S/pyNL6P0VU2B/eS8kQNZ1ACp5/k8hRSn8\n\
A5nCsPtNXxyFAe9+1se95QKBgQCJtKF3YvqeC/qG7ddHESupf1itn8dGiMRb6whF\n\
smhGZ5/ipz/UziebwEbDQJaxxQwOpw7ouEofd9DCcIS8Xd382KzmCPqidqBiOPSq\n\
zQsicWfr9x94PzsPmDw1dI54Vm9uBc0ALYnfpXN/Br5xIkS7NJBq62tXnheSN04L\n\
uvvkcQKBgQDBWVdvXiZMjF5405bBvophiMLbNDlduAfH+RVgOy/k3gDK9I8J6UbK\n\
5jLUPh2x1+03DllJ/+Axae3HNRpjAL9Sel9vOGyp1A9YW6aD03p0DO1foKMKBaRM\n\
IYjBFg+ez+rlZ3w3INSJ9Ze6aNmoK3oPfr9j0t+6SKVbuwurAgTlgQ==\n\
-----END RSA PRIVATE KEY-----"

#define CONFIGURATION_DATA_1 "{\n\
  \"version\": 1,\n\
	\"deviceId\": \"Calypso-129001293\",\n\
	\"scopeId\": \"0ne006E0511\",\n\
	\"DPSServer\": \"global.azure-devices-provisioning.net\",\n\
	\"modelId\": \"dtmi:wuerthelektronik:designkit:calypsoiotkit;1\",\n\
	\"SNTPServer\": \"0.de.pool.ntp.org\",\n\
	\"timezone\": \"60\",\n\
  \"WiFiSSID\": \"SSID\",\n\
  \"WiFiPassword\": \"password\",\n\
  \"WiFiSecurity\":3\n\
}"

#define CONFIGURATION_DATA_2 "{\n\
  \"version\": 2,\n\
	\"deviceId\": \"calypso-iot-kit-1\",\n\
  \"awsendpoint\": \"a2m4kdan3jizyu-ats.iot.us-east-1.amazonaws.com\",\n\
	\"SNTPServer\": \"0.de.pool.ntp.org\",\n\
  \"timezone\": \"60\",\n\
  \"WiFiSSID\": \"WE_backup\",\n\
  \"WiFiPassword\": \"079429455001\",\n\
  \"WiFiSecurity\":3\n\
}"

#define AZURE_IOT_PNP_CONFIG_VERSION 1
#define AWS_CONFIG_VERSION 2
  // char awsendpoint[AWS_ENDPOINT_MAX_LEN];
  // uint8_t modeOfOperation = AWS_CONFIG_VERSION;

/*Parameters from the IoT central application*/
#define SCOPE_ID "0aa000A0000"
#define MODEL_ID "dtmi:wuerthelektronik:designkit:calypsoiotkit;1"

/*File path to certificate files stored on Calypso internal storage*/
#define ROOT_CA_PATH "user/azrootca"
#define ROOT_CA_1_PATH "user/azrootca1"
#define DEVICE_CERT_PATH "user/azdevcert"
#define DEVICE_KEY_PATH "user/azdevkey"
#define DEVICE_IOT_HUB_ADDRESS "user/iotHubAddr"
#define CONFIG_FILE_PATH "user/azdevconf"

/*Wi-Fi settings*/
#define WI_FI_CONNECT_DELAY 5000UL

/*MQTT settings*/
#define DPS_SERVER_ADDRESS "global.azure-devices-provisioning.net"
#define MQTT_PORT_SECURE 8883
#define MQTT_TLS_VERSION "TLSV1_2"
#define MQTT_CIPHER "TLS_RSA_WITH_AES_256_CBC_SHA256"

#define MAX_URL_LEN 128
#define DEVICE_CLAIM_DURATION 180000

// MQTT Topics
#define DEVICE_TWIN_DESIRED_PROP_RES_TOPIC "$iothub/twin/PATCH/properties/desired/#"
#define DEVICE_TWIN_RES_TOPIC "$iothub/twin/res/#"
#define DIRECT_METHOD_TOPIC "$iothub/methods/POST/#"
#define DEVICE_TWIN_MESSAGE_PATCH "$iothub/twin/PATCH/properties/reported/?$rid="
#define DEVICE_TWIN_GET_TOPIC "$iothub/twin/GET/?$rid="

#define PROVISIONING_RESP_TOPIC "$dps/registrations/res/#"
#define PROVISIONING_REG_REQ_TOPIC "$dps/registrations/PUT/iotdps-register/?$rid="
#define PROVISIONING_STATUS_REQ_TOPIC "$dps/registrations/GET/iotdps-get-operationstatus/?$rid="

#define SNTP_TIMEZONE "+120"
#define SNTP_SERVER "0.de.pool.ntp.org"

#define DEFAULT_TELEMETRY_SEND_INTEVAL 30 // seconds
#define MAX_TELEMETRY_SEND_INTERVAL 600   // seconds
#define MIN_TELEMETRY_SEND_INTERVAL 3     // seconds

#define STATUS_SUCCESS 200
#define STATUS_UPDATE_IN_PROGRESS 202
#define STATUS_SET_BY_DEV 203
#define STATUS_BAD_REQUEST 400
#define STATUS_EXCEPTION 500
#define STATUS_CLOUD_SUCCESS 204
#define STATUS_TOO_MANY_REQUESTS 429

#define CALYPSO_FIRMWARE_MIN_MAJOR_VERSION 2
#define CALYPSO_FIRMWARE_MIN_MINOR_VERSION 2

  extern bool sensorsPresent;
  extern volatile unsigned long telemetrySendInterval;
  TypeSerial *Azure_Device_init(void *Debug, void *CalypsoSerial);
  void Azure_Device_writeConfigFiles();
  bool Azure_Device_isConfigured();
  bool Azure_Device_isConnectedToWiFi();
  bool Azure_Device_isProvisioned();
  void Azure_Device_MQTTConnect();
  void Azure_Device_readSensors();
  void Azure_Device_PublishSensorData();
  void Azure_Device_PublishProperties();
  void Azure_Device_connect_WiFi();
  void Azure_Device_disconnect_WiFi();
  void Azure_Device_WiFi_provisioning();
  bool Azure_Device_provision();
  void Azure_Device_configurationInProgress();
  bool Azure_Device_SubscribeToTopics();
  void Azure_Device_reset();
  void Azure_Device_restart();
  bool Azure_Device_isStatusOK();
  void Azure_Device_processCloudMessage();
  bool Azure_Device_ConfigurationComplete();
  void Azure_Device_displaySensorData();
  bool Azure_Device_isUpToDate();
  unsigned long Azure_Device_getTelemetrySendInterval();
  bool Azure_Device_isSensorsPresent();
#ifdef __cplusplus
}
#endif

#endif /* P_N_P_DEVICE_H */