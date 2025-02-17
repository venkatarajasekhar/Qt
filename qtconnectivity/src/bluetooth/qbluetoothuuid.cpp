/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbluetoothuuid.h"
#include "qbluetoothservicediscoveryagent.h"

#include <QStringList>
#include <QtEndian>

#include <string.h>

QT_BEGIN_NAMESPACE

// Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB
Q_GLOBAL_STATIC_WITH_ARGS(QUuid, baseUuid, ("{00000000-0000-1000-8000-00805F9B34FB}"))

/*!
    \class QBluetoothUuid
    \inmodule QtBluetooth
    \brief The QBluetoothUuid class generates a UUID for each Bluetooth
    service.

    \since 5.2
*/

/*!
    \enum QBluetoothUuid::ProtocolUuid

    This enum is a convienience type for Bluetooth protocol UUIDs. Values of this type will be
    implicitly converted into a QBluetoothUuid when necessary.

    \value Sdp      SDP protocol UUID
    \value Udp      UDP protocol UUID
    \value Rfcomm   RFCOMM protocol UUID
    \value Tcp      TCP protocol UUID
    \value TcsBin   Telephony Control Specification UUID
    \value TcsAt    Telephony Control Specification AT UUID
    \value Att      Attribute protocol UUID
    \value Obex     OBEX protocol UUID
    \value Ip       IP protocol UUID
    \value Ftp      FTP protocol UUID
    \value Http     HTTP protocol UUID
    \value Wsp      WSP UUID
    \value Bnep     Bluetooth Network Encapsulation Protocol UUID
    \value Upnp     Extended Service Discovery Profile UUID
    \value Hidp     Human Interface Device Profile UUID
    \value HardcopyControlChannel Hardcopy Cable Replacement Profile UUID
    \value HardcopyDataChannel Hardcopy Cable Replacement Profile UUID
    \value HardcopyNotification Hardcopy Cable Replacement Profile UUID
    \value Avctp    Audio/Video Control Transport Protocol UUID
    \value Avdtp    Audio/Video Distribution Transport Protocol UUID
    \value Cmtp     Common ISDN Access Profile
    \value UdiCPlain UDI protocol UUID
    \value McapControlChannel Multi-Channel Adaptation Protocol UUID
    \value McapDataChannel Multi-Channel Adaptation Protocol UUID
    \value L2cap    L2CAP protocol UUID

    \sa QBluetoothServiceInfo::ProtocolDescriptorList
*/

/*!
    \enum QBluetoothUuid::ServiceClassUuid

    This enum is a convienience type for Bluetooth service class and profile UUIDs. Values of this type will be
    implicitly converted into a QBluetoothUuid when necessary. Some UUIDs refer to service class ids, others to profile
    ids and some can be used as both. In general, profile UUIDs shall only be used in a
    \l QBluetoothServiceInfo::BluetoothProfileDescriptorList attribute and service class UUIDs shall only be used
    in a \l QBluetoothServiceInfo::ServiceClassIds attribute. If the UUID is marked as profile and service class UUID
    it can be used as a value for either of the above service attributes. Such a dual use has historical reasons
    but is no longer permissible for newer UUIDs.

    The list below explicitly states as what type each UUID shall be used. Bluetooth Low Energy related values
    starting with 0x18 were introduced by Qt 5.4

    \value ServiceDiscoveryServer     Service discovery server UUID (service)
    \value BrowseGroupDescriptor      Browser group descriptor (service)
    \value PublicBrowseGroup          Public browse group service class. Services which have the public
                                      browse group in their \l {QBluetoothServiceInfo::BrowseGroupList}{browse group list}
                                      are discoverable by the remote devices.
    \value SerialPort                 Serial Port Profile UUID (service & profile)
    \value LANAccessUsingPPP          LAN Access Profile UUID (service & profile)
    \value DialupNetworking           Dial-up Networking Profile UUID (service & profile)
    \value IrMCSync                   Synchronization Profile UUID (service & profile)
    \value ObexObjectPush             OBEX object push service UUID (service & profile)
    \value OBEXFileTransfer           File Transfer Profile (FTP) UUID (service & profile)
    \value IrMCSyncCommand            Synchronization Profile UUID (profile)
    \value Headset                    Headset Profile (HSP) UUID (service & profile)
    \value AudioSource                Advanced Audio Distribution Profile (A2DP) UUID (service)
    \value AudioSink                  Advanced Audio Distribution Profile (A2DP) UUID (service)
    \value AV_RemoteControlTarget     Audio/Video Remote Control Profile (AVRCP) UUID (service)
    \value AdvancedAudioDistribution  Advanced Audio Distribution Profile (A2DP) UUID (profile)
    \value AV_RemoteControl           Audio/Video Remote Control Profile (AVRCP) UUID (service & profile)
    \value AV_RemoteControlController Audio/Video Remote Control Profile UUID (service)
    \value HeadsetAG                  Headset Profile (HSP) UUID (service)
    \value PANU                       Personal Area Networking Profile (PAN) UUID (service & profile)
    \value NAP                        Personal Area Networking Profile (PAN) UUID (service & profile)
    \value GN                         Personal Area Networking Profile (PAN) UUID (service & profile)
    \value DirectPrinting             Basic Printing Profile (BPP) UUID (service)
    \value ReferencePrinting          Related to Basic Printing Profile (BPP) UUID (service)
    \value BasicImage                 Basic Imaging Profile (BIP) UUID (profile)
    \value ImagingResponder           Basic Imaging Profile (BIP) UUID (service)
    \value ImagingAutomaticArchive Basic Imaging Profile (BIP) UUID (service)
    \value ImagingReferenceObjects Basic Imaging Profile (BIP) UUID (service)
    \value Handsfree                  Hands-Free Profile (HFP) UUID (service & profile)
    \value HandsfreeAudioGateway      Hands-Free Audio Gateway (HFP) UUID (service)
    \value DirectPrintingReferenceObjectsService Basic Printing Profile (BPP) UUID (service)
    \value ReflectedUI                Basic Printing Profile (BPP) UUID (service)
    \value BasicPrinting              Basic Printing Profile (BPP) UUID (profile)
    \value PrintingStatus             Basic Printing Profile (BPP) UUID (service)
    \value HumanInterfaceDeviceService Human Interface Device (HID) UUID (service & profile)
    \value HardcopyCableReplacement Hardcopy Cable Replacement Profile (HCRP) (profile)
    \value HCRPrint                   Hardcopy Cable Replacement Profile (HCRP) (service)
    \value HCRScan                    Hardcopy Cable Replacement Profile (HCRP) (service)
    \value SIMAccess                  SIM Access Profile (SAP) UUID (service and profile)
    \value PhonebookAccessPCE         Phonebook Access Profile (PBAP) UUID (service)
    \value PhonebookAccessPSE         Phonebook Access Profile (PBAP) UUID (service)
    \value PhonebookAccess            Phonebook Access Profile (PBAP) (profile)
    \value HeadsetHS                  Headset Profile (HSP) UUID (service)
    \value MessageAccessServer        Message Access Profile (MAP) UUID (service)
    \value MessageNotificationServer  Message Access Profile (MAP) UUID (service)
    \value MessageAccessProfile       Message Access Profile (MAP) UUID (profile)
    \value GNSS                       Global Navigation Satellite System UUID (profile)
    \value GNSSServer                 Global Navigation Satellite System Server (UUID) (service)
    \value Display3D                  3D Synchronization Display UUID (service)
    \value Glasses3D                  3D Synchronization Glasses UUID (service)
    \value Synchronization3D          3D Synchronization UUID (profile)
    \value MPSProfile                 Multi-Profile Specification UUID (profile)
    \value MPSService                 Multi-Profile Specification UUID (service)
    \value PnPInformation             Device Identification (DID) UUID (service & profile)
    \value GenericNetworking          Generic networking UUID (service)
    \value GenericFileTransfer        Generic file transfer UUID (service)
    \value GenericAudio               Generic audio UUID (service)
    \value GenericTelephony           Generic telephone UUID (service)
    \value VideoSource                Video Distribution Profile (VDP) UUID (service)
    \value VideoSink                  Video Distribution Profile (VDP) UUID (service)
    \value VideoDistribution          Video Distribution Profile (VDP) UUID (profile)
    \value HDP                        Health Device Profile (HDP) UUID (profile)
    \value HDPSource                  Health Device Profile Source (HDP) UUID (service)
    \value HDPSink                    Health Device Profile Sink (HDP) UUID (service)
    \value GenericAccess              Generic access service for Bluetooth Low Energy devices UUID (service).
                                      It contains generic information about the device. All available Characteristics are readonly.
    \value GenericAttribute
    \value ImmediateAlert             Immediate Alert UUID (service). The service exposes a control point to allow a peer
                                      device to cause the device to immediately alert.
    \value LinkLoss                   Link Loss UUID (service). The service defines behavior when a link is lost between two devices.
    \value TxPower                    Transmission Power UUID (service). The service exposes a device’s current
                                      transmit power level when in a connection.
    \value CurrentTimeService         Current Time UUID (service). The service defines how the current time can be exposed using
                                      the Generic Attribute Profile (GATT).
    \value ReferenceTimeUpdateService Reference Time update UUID (service). The service defines how a client can request
                                      an update from a reference time source from a time server.
    \value NextDSTChangeService       Next DST change UUID (service). The service defines how the information about an
                                      upcoming DST change can be exposed.
    \value Glucose                    Glucose UUID (service). The service exposes glucose and other data from a glucose sensor
                                      for use in consumer and professional healthcare applications.
    \value HealthThermometer          Health Thermometer UUID (service). The Health Thermometer service exposes temperature
                                      and other data from a thermometer intended for healthcare and fitness applications.
    \value DeviceInformation          Device Information UUID (service). The Device Information Service exposes manufacturer
                                      and/or vendor information about a device.
    \value HeartRate                  Heart Rate UUID (service). The service exposes the heart rate and other data from a
                                      Heart Rate Sensor intended for fitness applications.
    \value PhoneAlertStatusService    Phone Alert Status UUID (service). The service exposes the phone alert status when
                                      in a connection.
    \value BatteryService             Battery UUID (service). The Battery Service exposes the state of a battery within a device.
    \value BloodPressure              Blood Pressure UUID (service). The service exposes blood pressure and other data from a blood pressure
                                      monitor intended for healthcare applications.
    \value AlertNotificationService   Alert Notification UUID (service). The Alert Notification service exposes alert
                                      information on a device.
    \value HumanInterfaceDevice       Human Interface UUID (service). The service exposes the HID reports and other HID data
                                      intended for HID Hosts and HID Devices.
    \value ScanParameters             Scan Parameters UUID (service). The Scan Parameters Service enables a GATT Server device
                                      to expose a characteristic for the GATT Client to write its scan interval and scan window
                                      on the GATT Server device.
    \value RunningSpeedAndCadence     Runnung Speed and Cadence UUID (service). The service exposes speed, cadence and other
                                      data from a Running Speed and Cadence Sensor intended for fitness applications.
    \value CyclingSpeedAndCadence     Cycling Speed and Cadence UUID (service). The service exposes speed-related and
                                      cadence-related data from a Cycling Speed and Cadence sensor intended for fitness
                                      applications.
    \value CyclingPower               Cycling Speed UUID (service). The service exposes power- and force-related data and
                                      optionally speed- and cadence-related data from a Cycling Power
                                      sensor intended for sports and fitness applications.
    \value LocationAndNavigation      Location Navigation UUID (service). The service exposes location and navigation-related
                                      data from a Location and Navigation sensor intended for outdoor activity applications.
*/

/*!
    \enum QBluetoothUuid::CharacteristicType
    \since 5.4

    This enum is a convienience type for Bluetooth low energy service characteristics class UUIDs. Values of this type
    will be implicitly converted into a QBluetoothUuid when necessary.

    \value AlertCategoryID               Categories of alerts/messages.
    \value AlertCategoryIDBitMask        Categories of alerts/messages.
    \value AlertLevel                    The level of an alert a device is to sound.
                                         If this level is changed while the alert is being sounded,
                                         the new level should take effect.
    \value AlertNotificationControlPoint Control point of the Alert Notification server.
                                         Client can write the command here to request the several
                                         functions toward the server.
    \value AlertStatus                   The Alert Status characteristic defines the Status of alert.
    \value Appearance                    The external appearance of this device. The values are composed
                                         of a category (10-bits) and sub-categories (6-bits).
    \value BatteryLevel                  The current charge level of a battery. 100% represents fully charged
                                         while 0% represents fully discharged.
    \value BloodPressureFeature          The Blood Pressure Feature characteristic is used to describe the supported
                                         features of the Blood Pressure Sensor.
    \value BloodPressureMeasurement      The Blood Pressure Measurement characteristic is a variable length structure
                                         containing a Flags field, a Blood Pressure Measurement Compound Value field,
                                         and contains additional fields such as Time Stamp, Pulse Rate and User ID
                                         as determined by the contents of the Flags field.
    \value BodySensorLocation
    \value BootKeyboardInputReport       The Boot Keyboard Input Report characteristic is used to transfer fixed format
                                         and length Input Report data between a HID Host operating in Boot Protocol Mode
                                         and a HID Service corresponding to a boot keyboard.
    \value BootKeyboardOutputReport      The Boot Keyboard Output Report characteristic is used to transfer fixed format
                                         and length Output Report data between a HID Host operating in Boot Protocol Mode
                                         and a HID Service corresponding to a boot keyboard.
    \value BootMouseInputReport          The Boot Mouse Input Report characteristic is used to transfer fixed format and
                                         length Input Report data between a HID Host operating in Boot Protocol Mode and
                                         a HID Service corresponding to a boot mouse.
    \value CSCFeature                    The CSC (Cycling Speed and Cadence) Feature characteristic is used to describe
                                         the supported features of the Server.
    \value CSCMeasurement                The CSC Measurement characteristic (CSC refers to Cycling Speed and Cadence)
                                         is a variable length structure containing a Flags field and, based on the contents
                                         of the Flags field, may contain one or more additional fields as shown in the tables
                                         below.
    \value CurrentTime
    \value CyclingPowerControlPoint      The Cycling Power Control Point characteristic is used to request a specific function
                                         to be executed on the receiving device.
    \value CyclingPowerFeature           The CP Feature characteristic is used to report a list of features supported by
                                         the device.
    \value CyclingPowerMeasurement       The Cycling Power Measurement characteristic is a variable length structure containing
                                         a Flags field, an Instantaneous Power field and, based on the contents of the Flags
                                         field, may contain one or more additional fields as shown in the table below.
    \value CyclingPowerVector            The Cycling Power Vector characteristic is a variable length structure containing
                                         a Flags fieldand based on the contents of the Flags field, may contain one or more
                                         additional fields as shown in the table below.
    \value DateTime                      The Date Time characteristic is used to represent time.
    \value DayDateTime
    \value DayOfWeek
    \value DeviceName
    \value DSTOffset
    \value ExactTime256
    \value FirmwareRevisionString        The value of this characteristic is a UTF-8 string representing the firmware revision
                                         for the firmware within the device.
    \value GlucoseFeature                The Glucose Feature characteristic is used to describe the supported features
                                         of the Server. When read, the Glucose Feature characteristic returns a value
                                         that is used by a Client to determine the supported features of the Server.
    \value GlucoseMeasurement            The Glucose Measurement characteristic is a variable length structure containing
                                         a Flags field, a Sequence Number field, a Base Time field and, based upon the contents
                                         of the Flags field, may contain a Time Offset field, Glucose Concentration field,
                                         Type-Sample Location field and a Sensor Status Annunciation field.
    \value GlucoseMeasurementContext
    \value HardwareRevisionString        The value of this characteristic is a UTF-8 string representing the hardware revision
                                         for the hardware within the device.
    \value HeartRateControlPoint
    \value HeartRateMeasurement
    \value HIDControlPoint               The HID Control Point characteristic is a control-point attribute that defines the
                                         HID Commands when written.
    \value HIDInformation                The HID Information Characteristic returns the HID attributes when read.
    \value IEEE1107320601RegulatoryCertificationDataList The value of the characteristic is an opaque structure listing
                                         various regulatory and/or certification compliance items to which the device
                                         claims adherence.
    \value IntermediateCuffPressure      This characteristic has the same format as the Blood Pressure Measurement
                                         characteristic.
    \value IntermediateTemperature       The Intermediate Temperature characteristic has the same format as the
                                         Temperature Measurement characteristic
    \value LNControlPoint                The LN Control Point characteristic is used to request a specific function
                                         to be executed on the receiving device.
    \value LNFeature                     The LN Feature characteristic is used to report a list of features supported
                                         by the device.
    \value LocalTimeInformation
    \value LocationAndSpeed              The Location and Speed characteristic is a variable length structure containing
                                         a Flags field and, based on the contents of the Flags field, may contain a combination
                                         of data fields.
    \value ManufacturerNameString        The value of this characteristic is a UTF-8 string representing the name of the
                                         manufacturer of the device.
    \value MeasurementInterval           The Measurement Interval characteristic defines the time between measurements.
    \value ModelNumberString             The value of this characteristic is a UTF-8 string representing the model number
                                         assigned by the device vendor.
    \value Navigation                    The Navigation characteristic is a variable length structure containing a Flags field,
                                         a Bearing field, a Heading field and, based on the contents of the Flags field.
    \value NewAlert                      This characteristic defines the category of the alert and how many new alerts of
                                         that category have occurred in the server device.
    \value PeripheralPreferredConnectionParameters
    \value PeripheralPrivacyFlag
    \value PnPID                         The PnP_ID characteristic returns its value when read using the GATT Characteristic
                                         Value Read procedure.
    \value PositionQuality               The Position Quality characteristic is a variable length structure containing a
                                         Flags field and at least one of the optional data
    \value ProtocolMode                  The Protocol Mode characteristic is used to expose the current protocol mode of
                                         the HID Service with which it is associated, or to set the desired protocol
                                         mode of the HID Service.
    \value ReconnectionAddress           The Information included in this page is informative. The normative descriptions
                                         are contained in the applicable specification.
    \value RecordAccessControlPoint      This control point is used with a service to provide basic management functionality
                                         for the Glucose Sensor patient record database.
    \value ReferenceTimeInformation
    \value Report                        The Report characteristic is used to exchange data between a HID Device and a HID Host.
    \value ReportMap                     Only a single instance of this characteristic exists as part of a HID Service.
    \value RingerControlPoint            The Ringer Control Point characteristic defines the Control Point of Ringer.
    \value RingerSetting                 The Ringer Setting characteristic defines the Setting of the Ringer.
    \value RSCFeature                    The RSC (Running Speed and Cadence) Feature characteristic is used to describe the
                                         supported features of the Server.
    \value RSCMeasurement                RSC refers to Running Speed and Cadence.
    \value SCControlPoint                The SC Control Point characteristic is used to request a specific function to be
                                         executed on the receiving device.
    \value ScanIntervalWindow            The Scan Interval Window characteristic is used to store the scan parameters of
                                         the GATT Client.
    \value ScanRefresh                   The Scan Refresh characteristic is used to notify the Client that the Server
                                         requires the Scan Interval Window characteristic to be written with the latest
                                         values upon notification.
    \value SensorLocation                The Sensor Location characteristic is used to expose the location of the sensor.
    \value SerialNumberString            The value of this characteristic is a variable-length UTF-8 string representing
                                         the serial number for a particular instance of the device.
    \value ServiceChanged
    \value SoftwareRevisionString        The value of this characteristic is a UTF-8 string representing the software
                                         revision for the software within the device.
    \value SupportedNewAlertCategory     Category that the server supports for new alert.
    \value SupportedUnreadAlertCategory  Category that the server supports for unread alert.
    \value SystemID                      If the system ID is based of a Bluetooth Device Address with a Company Identifier
                                         (OUI) is 0x123456 and the Company Assigned Identifier is 0x9ABCDE, then the System
                                         Identifier is required to be 0x123456FFFE9ABCDE.
    \value TemperatureMeasurement        The Temperature Measurement characteristic is a variable length structure containing
                                         a Flags field, a Temperature Measurement Value field and, based upon the contents of
                                         the Flags field, optionally a Time Stamp field and/or a Temperature Type field.
    \value TemperatureType               The Temperature Type characteristic is an enumeration that indicates where the
                                         temperature was measured.
    \value TimeAccuracy
    \value TimeSource
    \value TimeUpdateControlPoint
    \value TimeUpdateState
    \value TimeWithDST
    \value TimeZone
    \value TxPowerLevel                  The value of the characteristic is a signed 8 bit integer that has a fixed point
                                         exponent of 0.
    \value UnreadAlertStatus             This characteristic shows how many numbers of unread alerts exist in the specific
                                         category in the device.
*/

/*!
    \enum QBluetoothUuid::DescriptorType
    \since 5.4

    Descriptors are attributes that describe Bluetooth Low Energy characteristic values.

    This enum is a convienience type for descriptor class UUIDs. Values of this type
    will be implicitly converted into a QBluetoothUuid when necessary.

    \value CharacteristicExtendedProperties  Descriptor defines additional Characteristic Properties.
                                             The existence of this descriptor is indicated by the
                                             \l QLowEnergyCharacteristic::ExtendedProperty flag.
    \value CharacteristicUserDescription     Descriptor provides a textual user description for a characteristic value.
    \value ClientCharacteristicConfiguration Descriptor defines how the characteristic may be configured by a specific client.
    \value ServerCharacteristicConfiguration Descriptor defines how the characteristic descriptor is associated with may be
                                             configured for the server.
    \value CharacteristicPresentationFormat  Descriptor defines the format of the Characteristic Value.
    \value CharacteristicAggregateFormat     Descriptor defines the format of an aggregated Characteristic Value.
    \value ValidRange                        descriptor is used for defining the range of a characteristics.
                                             Two mandatory fields are contained (upper and lower bounds) which define the range.
    \value ExternalReportReference           Allows a HID Host to map information from the Report Map characteristic value for
                                             Input Report, Output Report or Feature Report data to the Characteristic UUID of
                                             external service characteristics used to transfer the associated data.
    \value ReportReference                   Mapping information in the form of a Report ID and Report Type which maps the
                                             current parent characteristic to the Report ID(s) and Report Type (s) defined
                                             within the Report Map characteristic.
    \value UnknownDescriptorType             The descriptor type is unknown.
*/

/*!
    Constructs a new null Bluetooth UUID.
*/
QBluetoothUuid::QBluetoothUuid()
{
}

/*!
    Constructs a new Bluetooth UUID from the protocol \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(ProtocolUuid uuid)
:   QUuid(uuid, baseUuid()->data2,
          baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
          baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
          baseUuid()->data4[6], baseUuid()->data4[7])
{
}

/*!
    Constructs a new Bluetooth UUID from the service class \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(ServiceClassUuid uuid)
:   QUuid(uuid, baseUuid()->data2, baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
          baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
          baseUuid()->data4[6], baseUuid()->data4[7])
{
}

/*!
    Constructs a new Bluetooth UUID from the characteristic type \a uuid.
    \since 5.4
*/
QBluetoothUuid::QBluetoothUuid(CharacteristicType uuid)
:   QUuid(uuid, baseUuid()->data2, baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
          baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
          baseUuid()->data4[6], baseUuid()->data4[7])
{
}

/*!
    Constructs a new Bluetooth UUID from the descriptor type \a uuid.
    \since 5.4
*/
QBluetoothUuid::QBluetoothUuid(DescriptorType uuid)
    :   QUuid(uuid, baseUuid()->data2, baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
              baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
              baseUuid()->data4[6], baseUuid()->data4[7])
{

}

/*!
    Constructs a new Bluetooth UUID from the 16 bit \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(quint16 uuid)
:   QUuid(uuid, baseUuid()->data2, baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
          baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
          baseUuid()->data4[6], baseUuid()->data4[7])
{
}

/*!
    Constructs a new Bluetooth UUID from the 32 bit \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(quint32 uuid)
:   QUuid(uuid, baseUuid()->data2, baseUuid()->data3, baseUuid()->data4[0], baseUuid()->data4[1],
          baseUuid()->data4[2], baseUuid()->data4[3], baseUuid()->data4[4], baseUuid()->data4[5],
          baseUuid()->data4[6], baseUuid()->data4[7])
{
}

/*!
    Constructs a new Bluetooth UUID from the 128 bit \a uuid.

    Note that \a uuid must be in big endian order.
*/
QBluetoothUuid::QBluetoothUuid(quint128 uuid)
{
    data1 = qFromBigEndian<quint32>(*reinterpret_cast<quint32 *>(&uuid.data[0]));
    data2 = qFromBigEndian<quint16>(*reinterpret_cast<quint16 *>(&uuid.data[4]));
    data3 = qFromBigEndian<quint16>(*reinterpret_cast<quint16 *>(&uuid.data[6]));

    memcpy(data4, &uuid.data[8], 8);
}

/*!
    Constructs a new Bluetooth UUID from the \a uuid string.

    The string must be in the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX.
*/
QBluetoothUuid::QBluetoothUuid(const QString &uuid)
:   QUuid(uuid)
{
}

/*!
    Constructs a new Bluetooth UUID that is a copy of \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(const QBluetoothUuid &uuid)
:   QUuid(uuid)
{
}

/*!
    Constructs a new Bluetooth UUID that is a copy of \a uuid.
*/
QBluetoothUuid::QBluetoothUuid(const QUuid &uuid)
:   QUuid(uuid)
{
}

/*!
    Destroys the Bluetooth UUID.
*/
QBluetoothUuid::~QBluetoothUuid()
{
}

/*!
    Returns the minimum size in bytes that this UUID can be represented in.  For non-null UUIDs 2,
    4 or 16 is returned.  0 is returned for null UUIDs.

    \sa isNull(), toUInt16(), toUInt32(), toUInt128()
*/
int QBluetoothUuid::minimumSize() const
{
    if (data2 == baseUuid()->data2 && data3 == baseUuid()->data3 &&
        memcmp(data4, baseUuid()->data4, 8) == 0) {
        // 16 or 32 bit Bluetooth UUID
        if (data1 & 0xFFFF0000)
            return 4;
        else
            return 2;
    }

    if (isNull())
        return 0;

    return 16;
}

/*!
    Returns the 16 bit representation of this UUID. If \a ok is passed, it is set to true if the
    conversion is possible, otherwise it is set to false. The return value is undefined if \a ok is
    set to false.
*/
quint16 QBluetoothUuid::toUInt16(bool *ok) const
{
    if (data1 & 0xFFFF0000 || data2 != baseUuid()->data2 || data3 != baseUuid()->data3 ||
        memcmp(data4, baseUuid()->data4, 8) != 0) {
        // not convertable to 16 bit Bluetooth UUID.
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;

    return data1;
}

/*!
    Returns the 32 bit representation of this UUID. If \a ok is passed, it is set to true if the
    conversion is possible, otherwise it is set to false. The return value is undefined if \a ok is
    set to false.
*/
quint32 QBluetoothUuid::toUInt32(bool *ok) const
{
    if (data2 != baseUuid()->data2 || data3 != baseUuid()->data3 ||
        memcmp(data4, baseUuid()->data4, 8) != 0) {
        // not convertable to 32 bit Bluetooth UUID.
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;

    return data1;
}

/*!
    Returns the 128 bit representation of this UUID.
*/
quint128 QBluetoothUuid::toUInt128() const
{
    quint128 uuid;

    quint32 tmp32 = qToBigEndian<quint32>(data1);
    memcpy(&uuid.data[0], &tmp32, 4);

    quint16 tmp16 = qToBigEndian<quint16>(data2);
    memcpy(&uuid.data[4], &tmp16, 2);

    tmp16 = qToBigEndian<quint16>(data3);
    memcpy(&uuid.data[6], &tmp16, 2);

    memcpy(&uuid.data[8], data4, 8);

    return uuid;
}

/*!
    Returns a human-readable and translated name for the given service class
    represented by \a uuid.

    \sa QBluetoothUuid::ServiceClassUuid
    \since Qt 5.4
 */
QString QBluetoothUuid::serviceClassToString(QBluetoothUuid::ServiceClassUuid uuid)
{
    switch (uuid) {
    case QBluetoothUuid::ServiceDiscoveryServer: return QBluetoothServiceDiscoveryAgent::tr("Service Discovery");
    case QBluetoothUuid::BrowseGroupDescriptor: return QBluetoothServiceDiscoveryAgent::tr("Browse Group Descriptor");
    case QBluetoothUuid::PublicBrowseGroup: return QBluetoothServiceDiscoveryAgent::tr("Public Browse Group");
    case QBluetoothUuid::SerialPort: return QBluetoothServiceDiscoveryAgent::tr("Serial Port Profile");
    case QBluetoothUuid::LANAccessUsingPPP: return QBluetoothServiceDiscoveryAgent::tr("LAN Access Profile");
    case QBluetoothUuid::DialupNetworking: return QBluetoothServiceDiscoveryAgent::tr("Dial-Up Networking");
    case QBluetoothUuid::IrMCSync: return QBluetoothServiceDiscoveryAgent::tr("Synchronization");
    case QBluetoothUuid::ObexObjectPush: return QBluetoothServiceDiscoveryAgent::tr("Object Push");
    case QBluetoothUuid::OBEXFileTransfer: return QBluetoothServiceDiscoveryAgent::tr("File Transfer");
    case QBluetoothUuid::IrMCSyncCommand: return QBluetoothServiceDiscoveryAgent::tr("Synchronization Command");
    case QBluetoothUuid::Headset: return QBluetoothServiceDiscoveryAgent::tr("Headset");
    case QBluetoothUuid::AudioSource: return QBluetoothServiceDiscoveryAgent::tr("Audio Source");
    case QBluetoothUuid::AudioSink: return QBluetoothServiceDiscoveryAgent::tr("Audio Sink");
    case QBluetoothUuid::AV_RemoteControlTarget: return QBluetoothServiceDiscoveryAgent::tr("Audio/Video Remote Control Target");
    case QBluetoothUuid::AdvancedAudioDistribution: return QBluetoothServiceDiscoveryAgent::tr("Advanced Audio Distribution");
    case QBluetoothUuid::AV_RemoteControl: return QBluetoothServiceDiscoveryAgent::tr("Audio/Video Remote Control");
    case QBluetoothUuid::AV_RemoteControlController: return QBluetoothServiceDiscoveryAgent::tr("Audio/Video Remote Control Controller");
    case QBluetoothUuid::HeadsetAG: return QBluetoothServiceDiscoveryAgent::tr("Headset AG");
    case QBluetoothUuid::PANU: return QBluetoothServiceDiscoveryAgent::tr("Personal Area Networking (PANU)");
    case QBluetoothUuid::NAP: return QBluetoothServiceDiscoveryAgent::tr("Personal Area Networking (NAP)");
    case QBluetoothUuid::GN: return QBluetoothServiceDiscoveryAgent::tr("Personal Area Networking (GN)");
    case QBluetoothUuid::DirectPrinting: return QBluetoothServiceDiscoveryAgent::tr("Basic Direct Printing (BPP)");
    case QBluetoothUuid::ReferencePrinting: return QBluetoothServiceDiscoveryAgent::tr("Basic Reference Printing (BPP)");
    case QBluetoothUuid::BasicImage: return QBluetoothServiceDiscoveryAgent::tr("Basic Imaging Profile");
    case QBluetoothUuid::ImagingResponder: return QBluetoothServiceDiscoveryAgent::tr("Basic Imaging Responder");
    case QBluetoothUuid::ImagingAutomaticArchive: return QBluetoothServiceDiscoveryAgent::tr("Basic Imaging Archive");
    case QBluetoothUuid::ImagingReferenceObjects: return QBluetoothServiceDiscoveryAgent::tr("Basic Imaging Ref Objects");
    case QBluetoothUuid::Handsfree: return QBluetoothServiceDiscoveryAgent::tr("Hands-Free");
    case QBluetoothUuid::HandsfreeAudioGateway: return QBluetoothServiceDiscoveryAgent::tr("Hands-Free AG");
    case QBluetoothUuid::DirectPrintingReferenceObjectsService: return QBluetoothServiceDiscoveryAgent::tr("Basic Printing RefObject Service");
    case QBluetoothUuid::ReflectedUI: return QBluetoothServiceDiscoveryAgent::tr("Basic Printing Reflected UI");
    case QBluetoothUuid::BasicPrinting: return QBluetoothServiceDiscoveryAgent::tr("Basic Printing");
    case QBluetoothUuid::PrintingStatus: return QBluetoothServiceDiscoveryAgent::tr("Basic Printing Status");
    case QBluetoothUuid::HumanInterfaceDeviceService: return QBluetoothServiceDiscoveryAgent::tr("Human Interface Device");
    case QBluetoothUuid::HardcopyCableReplacement: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Cable Replacement");
    case QBluetoothUuid::HCRPrint: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Cable Replacement Print");
    case QBluetoothUuid::HCRScan: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Cable Replacement Scan");
    case QBluetoothUuid::SIMAccess: return QBluetoothServiceDiscoveryAgent::tr("SIM Access Server");
    case QBluetoothUuid::PhonebookAccessPCE: return QBluetoothServiceDiscoveryAgent::tr("Phonebook Access PCE");
    case QBluetoothUuid::PhonebookAccessPSE: return QBluetoothServiceDiscoveryAgent::tr("Phonebook Access PSE");
    case QBluetoothUuid::PhonebookAccess: return QBluetoothServiceDiscoveryAgent::tr("Phonebook Access");
    case QBluetoothUuid::HeadsetHS: return QBluetoothServiceDiscoveryAgent::tr("Headset HS");
    case QBluetoothUuid::MessageAccessServer: return QBluetoothServiceDiscoveryAgent::tr("Message Access Server");
    case QBluetoothUuid::MessageNotificationServer: return QBluetoothServiceDiscoveryAgent::tr("Message Notification Server");
    case QBluetoothUuid::MessageAccessProfile: return QBluetoothServiceDiscoveryAgent::tr("Message Access");
    case QBluetoothUuid::GNSS: return QBluetoothServiceDiscoveryAgent::tr("Global Navigation Satellite System");
    case QBluetoothUuid::GNSSServer: return QBluetoothServiceDiscoveryAgent::tr("Global Navigation Satellite System Server");
    case QBluetoothUuid::Display3D: return QBluetoothServiceDiscoveryAgent::tr("3D Synchronization Display");
    case QBluetoothUuid::Glasses3D: return QBluetoothServiceDiscoveryAgent::tr("3D Synchronization Glasses");
    case QBluetoothUuid::Synchronization3D: return QBluetoothServiceDiscoveryAgent::tr("3D Synchronization");
    case QBluetoothUuid::MPSProfile: return QBluetoothServiceDiscoveryAgent::tr("Multi-Profile Specification (Profile)");
    case QBluetoothUuid::MPSService: return QBluetoothServiceDiscoveryAgent::tr("Multi-Profile Specification");
    case QBluetoothUuid::PnPInformation: return QBluetoothServiceDiscoveryAgent::tr("Device Identification");
    case QBluetoothUuid::GenericNetworking: return QBluetoothServiceDiscoveryAgent::tr("Generic Networking");
    case QBluetoothUuid::GenericFileTransfer: return QBluetoothServiceDiscoveryAgent::tr("Generic File Transfer");
    case QBluetoothUuid::GenericAudio: return QBluetoothServiceDiscoveryAgent::tr("Generic Audio");
    case QBluetoothUuid::GenericTelephony: return QBluetoothServiceDiscoveryAgent::tr("Generic Telephony");
    case QBluetoothUuid::VideoSource: return QBluetoothServiceDiscoveryAgent::tr("Video Source");
    case QBluetoothUuid::VideoSink: return QBluetoothServiceDiscoveryAgent::tr("Video Sink");
    case QBluetoothUuid::VideoDistribution: return QBluetoothServiceDiscoveryAgent::tr("Video Distribution");
    case QBluetoothUuid::HDP: return QBluetoothServiceDiscoveryAgent::tr("Health Device");
    case QBluetoothUuid::HDPSource: return QBluetoothServiceDiscoveryAgent::tr("Health Device Source");
    case QBluetoothUuid::HDPSink: return QBluetoothServiceDiscoveryAgent::tr("Health Device Sink");
    case QBluetoothUuid::GenericAccess: return QBluetoothServiceDiscoveryAgent::tr("Generic Access");
    case QBluetoothUuid::GenericAttribute: return QBluetoothServiceDiscoveryAgent::tr("Generic Attribute");
    case QBluetoothUuid::ImmediateAlert: return QBluetoothServiceDiscoveryAgent::tr("Immediate Alert");
    case QBluetoothUuid::LinkLoss: return QBluetoothServiceDiscoveryAgent::tr("Link Loss");
    case QBluetoothUuid::TxPower: return QBluetoothServiceDiscoveryAgent::tr("Tx Power");
    case QBluetoothUuid::CurrentTimeService: return QBluetoothServiceDiscoveryAgent::tr("Current Time Service");
    case QBluetoothUuid::ReferenceTimeUpdateService: return QBluetoothServiceDiscoveryAgent::tr("Reference Time Update Service");
    case QBluetoothUuid::NextDSTChangeService: return QBluetoothServiceDiscoveryAgent::tr("Next DST Change Service");
    case QBluetoothUuid::Glucose: return QBluetoothServiceDiscoveryAgent::tr("Glucose");
    case QBluetoothUuid::HealthThermometer: return QBluetoothServiceDiscoveryAgent::tr("Health Thermometer");
    case QBluetoothUuid::DeviceInformation: return QBluetoothServiceDiscoveryAgent::tr("Device Information");
    case QBluetoothUuid::HeartRate: return QBluetoothServiceDiscoveryAgent::tr("Heart Rate");
    case QBluetoothUuid::PhoneAlertStatusService: return QBluetoothServiceDiscoveryAgent::tr("Phone Alert Status Service");
    case QBluetoothUuid::BatteryService: return QBluetoothServiceDiscoveryAgent::tr("Battery Service");
    case QBluetoothUuid::BloodPressure: return QBluetoothServiceDiscoveryAgent::tr("Blood Pressure");
    case QBluetoothUuid::AlertNotificationService: return QBluetoothServiceDiscoveryAgent::tr("Alert Notification Service");
    case QBluetoothUuid::HumanInterfaceDevice: return QBluetoothServiceDiscoveryAgent::tr("Human Interface Device");
    case QBluetoothUuid::ScanParameters: return QBluetoothServiceDiscoveryAgent::tr("Scan Parameters");
    case QBluetoothUuid::RunningSpeedAndCadence: return QBluetoothServiceDiscoveryAgent::tr("Running Speed and Cadance");
    case QBluetoothUuid::CyclingSpeedAndCadence: return QBluetoothServiceDiscoveryAgent::tr("Cycling Speed and Cadance");
    case QBluetoothUuid::CyclingPower: return QBluetoothServiceDiscoveryAgent::tr("Cycling Power");
    case QBluetoothUuid::LocationAndNavigation: return QBluetoothServiceDiscoveryAgent::tr("Location and Navigation");
    default:
        break;
    }

    return QString();
}


/*!
    Returns a human-readable and translated name for the given protocol
    represented by \a uuid.

    \sa QBluetoothUuid::ProtocolUuid

    \since 5.4
 */
QString QBluetoothUuid::protocolToString(QBluetoothUuid::ProtocolUuid uuid)
{
    switch (uuid) {
    case QBluetoothUuid::Sdp: return QBluetoothServiceDiscoveryAgent::tr("Service Discovery Protocol");
    case QBluetoothUuid::Udp: return QBluetoothServiceDiscoveryAgent::tr("User Datagram Protocol");
    case QBluetoothUuid::Rfcomm: return QBluetoothServiceDiscoveryAgent::tr("Radio Frequency Communication");
    case QBluetoothUuid::Tcp: return QBluetoothServiceDiscoveryAgent::tr("Transmission Control Protocol");
    case QBluetoothUuid::TcsBin: return QBluetoothServiceDiscoveryAgent::tr("Telephony Control Specification - Binary");
    case QBluetoothUuid::TcsAt: return QBluetoothServiceDiscoveryAgent::tr("Telephony Control Specification - AT");
    case QBluetoothUuid::Att: return QBluetoothServiceDiscoveryAgent::tr("Attribute Protocol");
    case QBluetoothUuid::Obex: return QBluetoothServiceDiscoveryAgent::tr("Object Exchange Protocol");
    case QBluetoothUuid::Ip: return QBluetoothServiceDiscoveryAgent::tr("Internet Protocol");
    case QBluetoothUuid::Ftp: return QBluetoothServiceDiscoveryAgent::tr("File Transfer Protocol");
    case QBluetoothUuid::Http: return QBluetoothServiceDiscoveryAgent::tr("Hypertext Transfer Protocol");
    case QBluetoothUuid::Wsp: return QBluetoothServiceDiscoveryAgent::tr("Wireless Short Packet Protocol");
    case QBluetoothUuid::Bnep: return QBluetoothServiceDiscoveryAgent::tr("Bluetooth Network Encapsulation Protocol");
    case QBluetoothUuid::Upnp: return QBluetoothServiceDiscoveryAgent::tr("Extended Service Discovery Protocol");
    case QBluetoothUuid::Hidp: return QBluetoothServiceDiscoveryAgent::tr("Human Interface Device Protocol");
    case QBluetoothUuid::HardcopyControlChannel: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Control Channel");
    case QBluetoothUuid::HardcopyDataChannel: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Data Channel");
    case QBluetoothUuid::HardcopyNotification: return QBluetoothServiceDiscoveryAgent::tr("Hardcopy Notification");
    case QBluetoothUuid::Avctp: return QBluetoothServiceDiscoveryAgent::tr("Audio/Video Control Transport Protocol");
    case QBluetoothUuid::Avdtp: return QBluetoothServiceDiscoveryAgent::tr("Audio/Video Distribution Transport Protocol");
    case QBluetoothUuid::Cmtp: return QBluetoothServiceDiscoveryAgent::tr("Common ISDN Access Protocol");
    case QBluetoothUuid::UdiCPlain: return QBluetoothServiceDiscoveryAgent::tr("UdiCPlain");
    case QBluetoothUuid::McapControlChannel: return QBluetoothServiceDiscoveryAgent::tr("Multi-Channel Adaptation Protocol -Conrol");
    case QBluetoothUuid::McapDataChannel: return QBluetoothServiceDiscoveryAgent::tr("Multi-Channel Adaptation Protocol - Data");
    case QBluetoothUuid::L2cap: return QBluetoothServiceDiscoveryAgent::tr("Layer 2 Control Protocol");
    default:
        break;
    }

    return QString();
}

/*!
    Returns a human-readable and translated name for the given characteristic type
    represented by \a uuid.

    \sa QBluetoothUuid::CharacteristicType

    \since 5.4
*/
QString QBluetoothUuid::characteristicToString(CharacteristicType uuid)
{
    switch (uuid) {
    case QBluetoothUuid::DeviceName: return QBluetoothServiceDiscoveryAgent::tr("GAP Device Name");
    case QBluetoothUuid::Appearance: return QBluetoothServiceDiscoveryAgent::tr("GAP Appearance");
    case QBluetoothUuid::PeripheralPrivacyFlag:
        return QBluetoothServiceDiscoveryAgent::tr("GAP Peripheral Privacy Flag");
    case QBluetoothUuid::ReconnectionAddress:
        return QBluetoothServiceDiscoveryAgent::tr("GAP Reconnection Address");
    case QBluetoothUuid::PeripheralPreferredConnectionParameters:
        return QBluetoothServiceDiscoveryAgent::tr("GAP Peripheral Preferred Connection Parameters");
    case QBluetoothUuid::ServiceChanged: return QBluetoothServiceDiscoveryAgent::tr("GATT Service Changed");
    case QBluetoothUuid::AlertLevel: return QBluetoothServiceDiscoveryAgent::tr("Alert Level");
    case QBluetoothUuid::TxPowerLevel: return QBluetoothServiceDiscoveryAgent::tr("TX Power");
    case QBluetoothUuid::DateTime: return QBluetoothServiceDiscoveryAgent::tr("Date Time");
    case QBluetoothUuid::DayOfWeek: return QBluetoothServiceDiscoveryAgent::tr("Day Of Week");
    case QBluetoothUuid::DayDateTime: return QBluetoothServiceDiscoveryAgent::tr("Day Date Time");
    case QBluetoothUuid::ExactTime256: return QBluetoothServiceDiscoveryAgent::tr("Exact Time 256");
    case QBluetoothUuid::DSTOffset: return QBluetoothServiceDiscoveryAgent::tr("DST Offset");
    case QBluetoothUuid::TimeZone: return QBluetoothServiceDiscoveryAgent::tr("Time Zone");
    case QBluetoothUuid::LocalTimeInformation:
        return QBluetoothServiceDiscoveryAgent::tr("Local Time Information");
    case QBluetoothUuid::TimeWithDST: return QBluetoothServiceDiscoveryAgent::tr("Time With DST");
    case QBluetoothUuid::TimeAccuracy: return QBluetoothServiceDiscoveryAgent::tr("Time Accuracy");
    case QBluetoothUuid::TimeSource: return QBluetoothServiceDiscoveryAgent::tr("Time Source");
    case QBluetoothUuid::ReferenceTimeInformation:
        return QBluetoothServiceDiscoveryAgent::tr("Reference Time Information");
    case QBluetoothUuid::TimeUpdateControlPoint:
        return QBluetoothServiceDiscoveryAgent::tr("Time Update Control Point");
    case QBluetoothUuid::TimeUpdateState: return QBluetoothServiceDiscoveryAgent::tr("Time Update State");
    case QBluetoothUuid::GlucoseMeasurement: return QBluetoothServiceDiscoveryAgent::tr("Glucose Measurement");
    case QBluetoothUuid::BatteryLevel: return QBluetoothServiceDiscoveryAgent::tr("Battery Level");
    case QBluetoothUuid::TemperatureMeasurement:
        return QBluetoothServiceDiscoveryAgent::tr("Temperature Measurement");
    case QBluetoothUuid::TemperatureType: return QBluetoothServiceDiscoveryAgent::tr("Temperature Type");
    case QBluetoothUuid::IntermediateTemperature:
        return QBluetoothServiceDiscoveryAgent::tr("Intermediate Temperature");
    case QBluetoothUuid::MeasurementInterval: return QBluetoothServiceDiscoveryAgent::tr("Measurement Interval");
    case QBluetoothUuid::BootKeyboardInputReport: return QBluetoothServiceDiscoveryAgent::tr("Boot Keyboard Input Report");
    case QBluetoothUuid::SystemID: return QBluetoothServiceDiscoveryAgent::tr("System ID");
    case QBluetoothUuid::ModelNumberString: return QBluetoothServiceDiscoveryAgent::tr("Model Number String");
    case QBluetoothUuid::SerialNumberString: return QBluetoothServiceDiscoveryAgent::tr("Serial Number String");
    case QBluetoothUuid::FirmwareRevisionString: return QBluetoothServiceDiscoveryAgent::tr("Firmware Revision String");
    case QBluetoothUuid::HardwareRevisionString: return QBluetoothServiceDiscoveryAgent::tr("Hardware Revision String");
    case QBluetoothUuid::SoftwareRevisionString: return QBluetoothServiceDiscoveryAgent::tr("Software Revision String");
    case QBluetoothUuid::ManufacturerNameString: return QBluetoothServiceDiscoveryAgent::tr("Manufacturer Name String");
    case QBluetoothUuid::IEEE1107320601RegulatoryCertificationDataList:
        return QBluetoothServiceDiscoveryAgent::tr("IEEE 11073 20601 Regulatory Certification Data List");
    case QBluetoothUuid::CurrentTime: return QBluetoothServiceDiscoveryAgent::tr("Current Time");
    case QBluetoothUuid::ScanRefresh: return QBluetoothServiceDiscoveryAgent::tr("Scan Refresh");
    case QBluetoothUuid::BootKeyboardOutputReport:
        return QBluetoothServiceDiscoveryAgent::tr("Boot Keyboard Output Report");
    case QBluetoothUuid::BootMouseInputReport: return QBluetoothServiceDiscoveryAgent::tr("Boot Mouse Input Report");
    case QBluetoothUuid::GlucoseMeasurementContext:
        return QBluetoothServiceDiscoveryAgent::tr("Glucose Measurement Context");
    case QBluetoothUuid::BloodPressureMeasurement:
        return QBluetoothServiceDiscoveryAgent::tr("Blood Pressure Measurement");
    case QBluetoothUuid::IntermediateCuffPressure:
        return QBluetoothServiceDiscoveryAgent::tr("Intermediate Cuff Pressure");
    case QBluetoothUuid::HeartRateMeasurement: return QBluetoothServiceDiscoveryAgent::tr("Heart Rate Measurement");
    case QBluetoothUuid::BodySensorLocation: return QBluetoothServiceDiscoveryAgent::tr("Body Sensor Location");
    case QBluetoothUuid::HeartRateControlPoint: return QBluetoothServiceDiscoveryAgent::tr("Heart Rate Control Point");
    case QBluetoothUuid::AlertStatus: return QBluetoothServiceDiscoveryAgent::tr("Alert Status");
    case QBluetoothUuid::RingerControlPoint: return QBluetoothServiceDiscoveryAgent::tr("Ringer Control Point");
    case QBluetoothUuid::RingerSetting: return QBluetoothServiceDiscoveryAgent::tr("Ringer Setting");
    case QBluetoothUuid::AlertCategoryIDBitMask:
        return QBluetoothServiceDiscoveryAgent::tr("Alert Category ID Bit Mask");
    case QBluetoothUuid::AlertCategoryID: return QBluetoothServiceDiscoveryAgent::tr("Alert Category ID");
    case QBluetoothUuid::AlertNotificationControlPoint:
        return QBluetoothServiceDiscoveryAgent::tr("Alert Notification Control Point");
    case QBluetoothUuid::UnreadAlertStatus: return QBluetoothServiceDiscoveryAgent::tr("Unread Alert Status");
    case QBluetoothUuid::NewAlert: return QBluetoothServiceDiscoveryAgent::tr("New Alert");
    case QBluetoothUuid::SupportedNewAlertCategory:
        return QBluetoothServiceDiscoveryAgent::tr("Supported New Alert Category");
    case QBluetoothUuid::SupportedUnreadAlertCategory:
        return QBluetoothServiceDiscoveryAgent::tr("Supported Unread Alert Category");
    case QBluetoothUuid::BloodPressureFeature: return QBluetoothServiceDiscoveryAgent::tr("Blood Pressure Feature");
    case QBluetoothUuid::HIDInformation: return QBluetoothServiceDiscoveryAgent::tr("HID Information");
    case QBluetoothUuid::ReportMap: return QBluetoothServiceDiscoveryAgent::tr("Report Map");
    case QBluetoothUuid::HIDControlPoint: return QBluetoothServiceDiscoveryAgent::tr("HID Control Point");
    case QBluetoothUuid::Report: return QBluetoothServiceDiscoveryAgent::tr("Report");
    case QBluetoothUuid::ProtocolMode: return QBluetoothServiceDiscoveryAgent::tr("Protocol Mode");
    case QBluetoothUuid::ScanIntervalWindow: return QBluetoothServiceDiscoveryAgent::tr("Scan Interval Window");
    case QBluetoothUuid::PnPID: return QBluetoothServiceDiscoveryAgent::tr("PnP ID");
    case QBluetoothUuid::GlucoseFeature: return QBluetoothServiceDiscoveryAgent::tr("Glucose Feature");
    case QBluetoothUuid::RecordAccessControlPoint:
        return QBluetoothServiceDiscoveryAgent::tr("Record Access Control Point");
    case QBluetoothUuid::RSCMeasurement: return QBluetoothServiceDiscoveryAgent::tr("RSC Measurement");
    case QBluetoothUuid::RSCFeature: return QBluetoothServiceDiscoveryAgent::tr("RSC Feature");
    case QBluetoothUuid::SCControlPoint: return QBluetoothServiceDiscoveryAgent::tr("SC Control Point");
    case QBluetoothUuid::CSCMeasurement: return QBluetoothServiceDiscoveryAgent::tr("CSC Measurement");
    case QBluetoothUuid::CSCFeature: return QBluetoothServiceDiscoveryAgent::tr("CSC Feature");
    case QBluetoothUuid::SensorLocation: return QBluetoothServiceDiscoveryAgent::tr("Sensor Location");
    case QBluetoothUuid::CyclingPowerMeasurement:
        return QBluetoothServiceDiscoveryAgent::tr("Cycling Power Measurement");
    case QBluetoothUuid::CyclingPowerVector: return QBluetoothServiceDiscoveryAgent::tr("Cycling Power Vector");
    case QBluetoothUuid::CyclingPowerFeature: return QBluetoothServiceDiscoveryAgent::tr("Cycling Power Feature");
    case QBluetoothUuid::CyclingPowerControlPoint:
        return QBluetoothServiceDiscoveryAgent::tr("Cycling Power COntrol Point");
    case QBluetoothUuid::LocationAndSpeed: return QBluetoothServiceDiscoveryAgent::tr("Location And Speed");
    case QBluetoothUuid::Navigation: return QBluetoothServiceDiscoveryAgent::tr("Navigation");
    case QBluetoothUuid::PositionQuality: return QBluetoothServiceDiscoveryAgent::tr("Position Quality");
    case QBluetoothUuid::LNFeature: return QBluetoothServiceDiscoveryAgent::tr("LN Feature");
    case QBluetoothUuid::LNControlPoint: return QBluetoothServiceDiscoveryAgent::tr("LN Control Point");
    default:
        break;
    }

    return QString();
}

/*!
    Returns a human-readable and translated name for the given descriptor type
    represented by \a uuid.

    \sa QBluetoothUuid::CharacteristicType

    \since 5.4
*/
QString QBluetoothUuid::descriptorToString(QBluetoothUuid::DescriptorType uuid)
{
    switch (uuid) {
    case QBluetoothUuid::CharacteristicExtendedProperties:
        return QBluetoothServiceDiscoveryAgent::tr("Characteristic Extended Properties");
    case QBluetoothUuid::CharacteristicUserDescription:
        return QBluetoothServiceDiscoveryAgent::tr("Characteristic User Description");
    case QBluetoothUuid::ClientCharacteristicConfiguration:
        return QBluetoothServiceDiscoveryAgent::tr("Client Characteristic Configuration");
    case QBluetoothUuid::ServerCharacteristicConfiguration:
        return QBluetoothServiceDiscoveryAgent::tr("Server Characteristic Configuratio");
    case QBluetoothUuid::CharacteristicPresentationFormat:
        return QBluetoothServiceDiscoveryAgent::tr("Characteristic Presentation Format");
    case QBluetoothUuid::CharacteristicAggregateFormat:
        return QBluetoothServiceDiscoveryAgent::tr("Characteristic Aggregate Format");
    case QBluetoothUuid::ValidRange:
        return QBluetoothServiceDiscoveryAgent::tr("Valid Range");
    case QBluetoothUuid::ExternalReportReference:
        return QBluetoothServiceDiscoveryAgent::tr("External Report Reference");
    case QBluetoothUuid::ReportReference:
        return QBluetoothServiceDiscoveryAgent::tr("Report Reference");
    default:
        break;
    }

    return QString();
}

/*!
    Returns true if \a other is equal to this Bluetooth UUID, otherwise false.
*/
bool QBluetoothUuid::operator==(const QBluetoothUuid &other) const
{
    return QUuid::operator==(other);
}

QT_END_NAMESPACE
