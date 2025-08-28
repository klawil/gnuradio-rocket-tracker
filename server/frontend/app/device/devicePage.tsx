'use client';

import { GetDeviceApi } from "@/types/DevicesApi";
import { defaultDeviceState, deviceReducer } from "@/utils/device";
import { timeToDelta, useTime } from "@/utils/time";
import { typeFetch } from "@/utils/typeFetch";
import { useSearchParams } from "next/navigation";
import { useCallback, useEffect, useReducer, useState } from "react";
import { Spinner, Tab, Table, Tabs } from "react-bootstrap";
import styles from './styles.module.css';
import { CircleMarker, MapContainer, Polyline } from "react-leaflet";
import { MapMarker, TileLayers, useUserLocation, ZoomTwoDevices } from "@/utils/map";

import 'leaflet/dist/leaflet.css';
import { useMessaging } from "@/utils/ws";

const igniterCutoff = 3.2; // Show red at <= 3.2 V

function DeviceRow({
  label,
  lastUpdated,
  currentTime,
  children,
  circle,
  circleGreen,
}: Readonly<{
  label: string;
  lastUpdated?: null | Date;
  currentTime?: number;
  children?: React.ReactNode;
  circle?: boolean;
  circleGreen?: boolean;
}>) {
  return <tr className='text-start'>
    <td className={styles.circleTd}>
      {circle && <>
        <span className={`${lastUpdated === null ? '' : circleGreen ? '' : styles.redCircle} me-1`}>{circleGreen || lastUpdated === null ? <>&#x25CB;</> : <>&#x25CF;</> }</span>
        <span className={`${lastUpdated === null ? '' : circleGreen ? styles.greenCircle : ''}`}>{circleGreen && lastUpdated !== null ? <>&#x25CF;</> : <>&#x25CB;</> }</span>
      </>}
    </td>
    <th>{label}:</th>
    <td>{lastUpdated === null ? '-' : children}</td>
    <td>{lastUpdated === null || typeof lastUpdated === 'undefined' ? '' : timeToDelta(currentTime || 0, lastUpdated.getTime())}</td>
  </tr>
}

export default function DevicePage() {
  const [ isLoading, setIsLoading ] = useState<boolean>(true);
  const [ error, setError ] = useState<null | string>(null);
  const [ device, dispatchDevice ] = useReducer(deviceReducer, defaultDeviceState);
  const searchParams = useSearchParams();
  const time = useTime();
  const userLoc = useUserLocation();

  const loadDevice = useCallback(async (serial: number) => {
    const [ code, response ] = await typeFetch<GetDeviceApi>({
      path: '/api/devices/{id}/',
      params: {
        id: serial,
      },
      method: 'GET',
    });

    setIsLoading(false);
    if (code !== 200 || response === null || !('Serial' in response)) {
      console.error(`Failed to get device ${serial}`, code, response);
      setError(`Failed to load ${serial}: code ${code}`);
      return;
    }

    // Init the device
    dispatchDevice({
      Type: 0,
      DeviceName: response.Name,
      Serial: response.Serial,
      StartPackets: response.Packets,
    });
  }, []);

  useEffect(() => {
    if (!searchParams.get('serial')) {
      return;
    }

    loadDevice(Number(searchParams.get('serial')));
  }, [
    searchParams,
    loadDevice,
  ]);

  const [ popMessage ] = useMessaging('ROCKET', Number(searchParams.get('serial')));
  useEffect(() => {
    const message = popMessage();
    if (typeof message !== 'undefined') {
      dispatchDevice(message);
    }
  }, [ popMessage, ]);

  if (isLoading) {
    return <>
      <h1>Loading... <Spinner /></h1>
    </>
  } else if (error !== null) {
    return <>
      <h1>Error Loading Device</h1>
      <h2>{error}</h2>
    </>
  }

  const apogeeIgniterRow = <DeviceRow
    circle={true}
    circleGreen={device.Igniters.Apogee > igniterCutoff}
    label='Apogee Igniter'
    lastUpdated={device.Igniters.LastUpdate}
    currentTime={time}
  >{device.Igniters.Apogee.toFixed(1)} V</DeviceRow>
  const mainIgniterRow = <DeviceRow
    circle={true}
    circleGreen={device.Igniters.Main > igniterCutoff}
    label='Main Igniter'
    lastUpdated={device.Igniters.LastUpdate}
    currentTime={time}
  >{device.Igniters.Main.toFixed(1)} V</DeviceRow>

  return <>
    <h1>Device {device.Name === null ? `Serial ${device.Serial}` : `${device.Name} (${device.Serial})`}</h1>
    <h4>{device.Config.DeviceName}</h4>

    <Table className='table-borderless text-start'>
      <thead className='small'>
        <tr className='small'>
          <td className='pb-0'>Call</td>
          <td className='pb-0'>Flight</td>
          <td className='pb-0'>State</td>
          <td className='pb-0'>Channel</td>
          <td className='pb-0'>Age</td>
        </tr>
      </thead>
      <tbody>
        <tr className='align-middle'>
          <td className='pt-0'>{device.Config.CallSign}</td>
          <td className='pt-0'>{device.Config.Flight}</td>
          <td className='pt-0'>{device.State.Name}</td>
          <td className='pt-0'>{device.Channel.toFixed(3)}</td>
          <td className='pt-0'>{timeToDelta(time, device.LastSeen.getTime())}</td>
        </tr>
      </tbody>
    </Table>

    <Tabs
      defaultActiveKey='pad'
      className='mb-3 nav-fill'
    >
      <Tab eventKey='pad' title='Pad'>
        <Table className='table-borderless table-striped'>
          <tbody>
            <DeviceRow
              circle={true}
              circleGreen={device.Battery.LastUpdate !== null}
              label='Battery'
              lastUpdated={device.Battery.LastUpdate}
              currentTime={time}
            >{device.Battery.Voltage.toFixed(2)} V</DeviceRow>
            <DeviceRow
              circle={true}
              circleGreen={device.Location.Locked && device.Location.LastUpdate !== null}
              label='GPS'
              lastUpdated={device.Location.LastUpdate}
              currentTime={time}
            >{device.Location.NumSats} sats</DeviceRow>
            {apogeeIgniterRow}
            <DeviceRow
              label='Apogee Delay'
              lastUpdated={device.Config.LastUpdate}
              currentTime={time}
            >{device.Config.ApogeeDelay.toFixed(1)} s</DeviceRow>
            {mainIgniterRow}
            <DeviceRow
              label='Main Deploy'
              lastUpdated={device.Config.LastUpdate}
              currentTime={time}
            >{device.Config.MainDeploy.toFixed(0)} m</DeviceRow>
          </tbody>
        </Table>
      </Tab>
      <Tab eventKey='flight' title='Flight'>
        <Table className='table-borderless table-striped'>
          <tbody>
            <DeviceRow
              label='Speed'
              lastUpdated={device.State.LastUpdate}
              currentTime={time}
            >{Math.round(device.State.Speed)} m/s</DeviceRow>
            <DeviceRow
              label='Max Speed'
            >{Math.round(device.Maximums.Speed)} m/s</DeviceRow>
            <DeviceRow
              label='Height'
              lastUpdated={device.State.LastUpdate}
              currentTime={time}
            >{Math.round(device.State.Height)} m</DeviceRow>
            <DeviceRow
              label='Max Height'
            >{Math.round(device.Maximums.Height)} m</DeviceRow>
            <DeviceRow
              label='Altitude'
              circle={true}
              circleGreen={device.Location.Locked && device.Location.LastUpdate !== null}
              lastUpdated={device.Location.LastUpdate}
              currentTime={time}
            >{Math.round(device.Location.Altitude)} m</DeviceRow>
            <DeviceRow
              label='Max Altitude'
            >{Math.round(device.Location.Altitude)} m</DeviceRow>
            {apogeeIgniterRow}
            {mainIgniterRow}
          </tbody>
        </Table>
      </Tab>
      <Tab eventKey='recover' title='Recover'>
        <Table className='table-borderless table-striped'>
          <tbody>
            <DeviceRow
              label='Max Speed'
            >{Math.round(device.Maximums.Speed)} m/s</DeviceRow>
            <DeviceRow
              label='Max Height'
            >{Math.round(device.Maximums.Height)} m</DeviceRow>
            <DeviceRow
              label='Max Altitude'
            >{Math.round(device.Location.Altitude)} m</DeviceRow>
          </tbody>
        </Table>
      </Tab>
    </Tabs>

    {device.Location.Locked && 
      <MapContainer
        className={styles.map}
        center={[
          device.Location.Latitude,
          device.Location.Longitude,
        ]}
        zoom={14}
        maxZoom={20}
      >
        <TileLayers />
        {userLoc !== null && <Polyline
          pathOptions={{ color: 'blue', }}
          positions={[
            [
              device.Location.Latitude,
              device.Location.Longitude,
            ],
            userLoc
          ]}
        />}
        <MapMarker device={device} />
        {userLoc !== null && <CircleMarker
          center={userLoc}
          radius={5}
          fillOpacity={1}
          opacity={0}
          fillColor='red'
          color='red'
        />}
        {userLoc !== null && <ZoomTwoDevices
          point1={[
            device.Location.Latitude,
            device.Location.Longitude,
          ]}
          point2={userLoc}
        />}
      </MapContainer>
    }
  </>;
}
